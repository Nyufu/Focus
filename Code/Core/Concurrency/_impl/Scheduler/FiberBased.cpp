#include "FiberBased.hpp"
#include <assert.hxx>
#include <algorithm.hxx>
#include <managed_object.hxx>
#include <Core/EngineConfig.hpp>


namespace Focus::Concurrency::_impl::Scheduler {

constexpr bool PageGuardUsed = config::debug;

struct ThreadArgs {
	FiberBased* const scheduler;
	HANDLE handle;
	long index;
};

using ManagedThreadArgs = STDEX managed_object<ThreadArgs, Memory::Allocator::Heap<ThreadArgs>>;

struct Thread {
	Thread(void* arg) noexcept;

	static void Spawn(ManagedThreadArgs& threadArgs) noexcept;

	static DWORD Entry(void* arg) noexcept;

	FiberBased* scheduler;
	HANDLE handle;
};

Thread::Thread(void* arg) noexcept {
	ASSERT(arg);
	ManagedThreadArgs threadArgs{ static_cast<ManagedThreadArgs::object_pointer>(arg) };

	Spawn(threadArgs);

	scheduler = threadArgs->scheduler;
	handle = threadArgs->handle;
}

void Thread::Spawn(ManagedThreadArgs& threadArgs) noexcept {
	if (threadArgs->index == 0)
		return;

	::_InterlockedDecrement(reinterpret_cast<volatile long*>(&threadArgs->index));

	DWORD threadId SUPPRESS_INITIALIZE(0);
	auto threadHandle = ::CreateThread(nullptr, 1, Thread::Entry, threadArgs.get_ptr(), CREATE_SUSPENDED, &threadId);

	if (!threadHandle)
		STD abort();

	auto* const scheduler = threadArgs->scheduler;
	ASSERT(scheduler);

	scheduler->threadHandles[threadArgs->index] = threadHandle;
	threadArgs->handle = threadHandle;

	::ResumeThread(threadHandle);
}

DWORD Thread::Entry(void* arg) noexcept {
	const Thread threadInfo{ arg };

	threadInfo.scheduler->ExecuteScheduler();

	return 0;
}

#pragma region FiberBased_Constructor
// clang-format off

namespace {

constexpr STD array<StackSize, enum_count<StackSize>> stackTypes{
	StackSize::Large,
	StackSize::Medium,
	StackSize::Small
};

}

FiberBased::FiberBased()
	: FiberBased(
		  STDEX align(Config::Engine.taskQueue.large.taskCount  * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up),
		  STDEX align(Config::Engine.taskQueue.medium.taskCount * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up),
		  STDEX align(Config::Engine.taskQueue.small.taskCount  * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up)) {
}

__forceinline FiberBased::FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes)
	: FiberBased(
		  largeSizeInBytes, mediumSizeInBytes, smallSizeInBytes, largeSizeInBytes + mediumSizeInBytes + smallSizeInBytes,
		  STD bit_ceil(STD max(
			  Config::Engine.taskQueue.large.taskCount + Config::Engine.taskQueue.medium.taskCount + Config::Engine.taskQueue.small.taskCount,
			  platform::registers_in_cacheline * platform::registers_in_cacheline)),
		  [] {
			  constexpr unsigned long long_max{ STD numeric_limits<long>::max() - 1 };
			  ASSERT(long_max > Config::Engine.numberOfThreads);
			  return Config::Engine.numberOfThreads;
		  }()) {
}

__forceinline FiberBased::FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, long numberOfThreads)
	: FiberBased(largeSizeInBytes, mediumSizeInBytes, smallSizeInBytes, freeSetSizeInBytes, queueSize, queueSize * enum_count<Priority> * platform::register_size, numberOfThreads) {
}

WARNING_SCOPE_BEGIN
CLANG_WARNING_DISABLE("-Wuninitialized") // Elements of freePools already used while still initializing.

__forceinline FiberBased::FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, size_t queueSizeInBytes, long numberOfThreads)
	: allocator{}
	, threadHandles(numberOfThreads)
	, queueBuffer{ allocator.allocate(freeSetSizeInBytes + queueSizeInBytes + platform::cacheline_size - platform::default_align) } // The default_align is the default align by NT allocator on x64.
	, freePoolSizesInBytes{ mediumSizeInBytes, largeSizeInBytes, smallSizeInBytes }
	, freePools{
		reinterpret_cast<FreeSetEntry*>(STDEX align(reinterpret_cast<uintptr_t>(queueBuffer)                           , platform::cacheline_size, STDEX align_way::up)),
		reinterpret_cast<FreeSetEntry*>(STDEX align(reinterpret_cast<uintptr_t>(freePools[0]) + freePoolSizesInBytes[0], platform::cacheline_size, STDEX align_way::up)),
		reinterpret_cast<FreeSetEntry*>(STDEX align(reinterpret_cast<uintptr_t>(freePools[1]) + freePoolSizesInBytes[1], platform::cacheline_size, STDEX align_way::up)) }
	, queueDesc{ queueSize }
	, queues{ [&] {
		STD remove_const_t<decltype(queues)> _queues;
		_queues[0] = reinterpret_cast<FiberHandle*>(
			STDEX align(reinterpret_cast<uintptr_t>(freePools[2]) + freePoolSizesInBytes[2]                  , platform::cacheline_size, STDEX align_way::up));
		_queues[1] = reinterpret_cast<FiberHandle*>(
			STDEX align(reinterpret_cast<uintptr_t>(_queues[0]) + (queueDesc.GetSize() * sizeof(FiberHandle)), platform::cacheline_size, STDEX align_way::up));
		_queues[2] = reinterpret_cast<FiberHandle*>(
			STDEX align(reinterpret_cast<uintptr_t>(_queues[1]) + (queueDesc.GetSize() * sizeof(FiberHandle)), platform::cacheline_size, STDEX align_way::up));
		return _queues;
	}() } {
	Init(freeSetSizeInBytes, queueSizeInBytes, numberOfThreads);
}

WARNING_SCOPE_END

__forceinline void FiberBased::Init(size_t freeSetSizeInBytes, size_t queueSizeInBytes, long numberOfThreads) {
	STD memset(freePools[0], 0, freeSetSizeInBytes);
	STD memset(queues[0], 0x01, queueSizeInBytes);

	const STD array<FreeSetEntry*, enum_count<StackSize>> freeSetIters{
		freePools[0],
		freePools[1],
		freePools[2]
	};

	{
		ManagedThreadArgs threadArgs({ numberOfThreads + 1 }, this, nullptr, numberOfThreads);
		Thread::Spawn(threadArgs);
	}

	const uint32_t sizeOfGuardPage = PageGuardUsed ? Config::SystemDefault.pageSize : 0;

	const STD array<size_t, enum_count<StackSize>> stackSizes{
		Config::Engine.taskQueue.large.stackSize  ? STDEX align(Config::Engine.taskQueue.large.stackSize,  Config::SystemDefault.pageSize, STDEX align_way::up) + sizeOfGuardPage : 0,
		Config::Engine.taskQueue.medium.stackSize ? STDEX align(Config::Engine.taskQueue.medium.stackSize, Config::SystemDefault.pageSize, STDEX align_way::up) + sizeOfGuardPage : 0,
		Config::Engine.taskQueue.small.stackSize  ? STDEX align(Config::Engine.taskQueue.small.stackSize,  Config::SystemDefault.pageSize, STDEX align_way::up) + sizeOfGuardPage : 0
	};

	const STD array<size_t, enum_count<StackSize>> counts{
		Config::Engine.taskQueue.large.taskCount,
		Config::Engine.taskQueue.medium.taskCount,
		Config::Engine.taskQueue.small.taskCount
	};

	const size_t stackPoolSize =
		stackSizes[0] * counts[0]
		+ stackSizes[1] * counts[1]
		+ stackSizes[2] * counts[2];

	stackPoolPtr = ::VirtualAlloc(nullptr, stackPoolSize, MEM_COMMIT, PAGE_READWRITE);
	ASSERT(stackPoolPtr);

	for (auto stackTypeIndex = 0Ui64, currentAddress = reinterpret_cast<uintptr_t>(stackPoolPtr); stackTypeIndex < enum_count<StackSize>; stackTypeIndex++) {
		const auto count = counts[stackTypeIndex];
		const auto stackSize = stackSizes[stackTypeIndex];
		const auto stackType = stackTypes[stackTypeIndex];

		auto freeSetIter = freeSetIters[stackTypeIndex];
		for (auto index = 0Ui64; index < count; index++, currentAddress += stackSize, freeSetIter++) {
			if constexpr (auto oldProtect = 0UL; PageGuardUsed) {
				const auto result = ::VirtualProtect(reinterpret_cast<void*>(currentAddress), sizeOfGuardPage, PAGE_NOACCESS, &oldProtect);
				ASSERT(result);
			}

			const auto stackLimit = currentAddress + sizeOfGuardPage;
			const auto preparedFiberStackAddress = currentAddress + stackSize - sizeof(FiberImpl);

			::new (reinterpret_cast<void*>(preparedFiberStackAddress)) FiberImpl{ .stackLimit = stackLimit, .stackSize = stackType, .freeSetPlace = freeSetIter };
			ptrdiff_t offset = preparedFiberStackAddress + sizeof(FiberImpl) - reinterpret_cast<uintptr_t>(stackPoolPtr);
			ASSERT((offset & 0xFFFFFFFF00000000) == 0ll);
			*freeSetIter = static_cast<uint32_t>(offset);
		}
	}
}

// clang-format on
#pragma endregion

FiberBased::~FiberBased() {
	const auto result = ::VirtualFree(stackPoolPtr, 0, MEM_RELEASE);
	ASSERT(result);
	allocator.deallocate(static_cast<uint8_t*>(queueBuffer), 1);
}

void FiberBased::Signal() noexcept {
	for (uint64_t localCounter = waitCounter.load(STD memory_order::relaxed);;) {
		if (!static_cast<uint8_t>(localCounter)) [[likely]]
			return;
		if (waitCounter.compare_exchange_strong(localCounter, localCounter - 0x0000000100000001)) {
			WakeByAddressSingle(STD addressof(waitCounter));
			return;
		}
	}
}

__forceinline void FiberBased::ExecuteScheduler() noexcept {
	struct TailState {
		FiberHandle handle;
		FiberHandle* address;
	};

	static constexpr STD array<Priority, enum_count<Priority>> priorities = { Priority::High, Priority::Normal, Priority::Low };

	const auto mask = queueDesc.GetMask();
	const auto redBlackMask = queueDesc.GetRedBlackMask();

	for (;;) {
		STD array<TailState, enum_count<Priority>> tailStates{};
		auto it = tailStates.begin();

		for (Priority prio : priorities) {
			auto& tail = GetValue(tails, prio);
			const auto queue = GetValue(queues, prio);

		RETRY:
			auto currentTail = tail.load(STD memory_order::relaxed);
			auto& valueRef = queue[currentTail & mask];
			const auto currentValue = reinterpret_cast<uintptr_t>(valueRef);
			const auto redBlackBit = static_cast<uint8_t>(currentValue);
			const uint8_t target = currentTail & redBlackMask ? 1 : 0;
			const auto nextTail = currentTail + 1;

			if (target == redBlackBit) {
				if (tail.compare_exchange_strong(currentTail, nextTail)) {
					const auto handle = reinterpret_cast<FiberHandle>(currentValue & 0xFFFFFFFFFFFFFF00);
					ExecuteFiber(handle);
					break;
				}
				goto RETRY;
			}

			it->handle = valueRef;
			it->address = STD addressof(valueRef);
			it++;
		}

		// STD atomic_thread_fence(STD memory_order::seq_cst);

		if (const auto& a = tailStates[0], &b = tailStates[1], &c = tailStates[2]; a.handle == *a.address || b.handle == *b.address || c.handle == *c.address) {
			auto counterValue = waitCounter.fetch_add(1, STD memory_order::acquire) + 1;
			WaitOnAddress(reinterpret_cast<volatile void*>(STD addressof(waitCounter)), &counterValue, sizeof(counterValue), INFINITE);
		}
	}
}

void FiberBased::ExecuteFiber(FiberHandle handle) noexcept {
	const auto fiber = static_cast<Fiber*>(handle);
	SwitchToFocusFiber(fiber);
	ReleaseFiber(handle);
}

}
