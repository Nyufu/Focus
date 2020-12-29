#include "FiberBased.hpp"
#include <assert.hxx>
#include <algorithm.hxx>
#include <managed_object.hxx>
#include <Core/Concurrency/_impl/Fiber.hpp>

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

	return 0;
}

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
		  STDEX align(Config::Engine.taskQueue.large.taskCount * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up),
		  STDEX align(Config::Engine.taskQueue.medium.taskCount * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up),
		  STDEX align(Config::Engine.taskQueue.small.taskCount * sizeof(FreeSetEntry), platform::cacheline_size, STDEX align_way::up)) {
}

FiberBased::FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes)
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


FiberBased::FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, long numberOfThreads)
	: allocator{}
	, threadHandles(numberOfThreads)
	, queueBuffer{ allocator.allocate(freeSetSizeInBytes + queueSize * 3 * platform::register_size +
		platform::cacheline_size - platform::default_align) } // The default_align is the default align by NT allocator on x64.
	, freeDesc{ FreeSetDescriptor{ mediumSizeInBytes, queueBuffer },
				FreeSetDescriptor{ largeSizeInBytes, freeDesc[0] },
				FreeSetDescriptor{ smallSizeInBytes, freeDesc[1] } }
	, queueDesc{ QueueDescriptor{ queueSize, freeDesc[2] },
				 QueueDescriptor{ queueDesc[0] },
				 QueueDescriptor{ queueDesc[1] } } {

	STD memset(freeDesc[0].storage, 0, freeSetSizeInBytes);

	const STD array<FreeSetEntry*, enum_count<StackSize>> freeSetIters{
		freeDesc[0].storage,
		freeDesc[1].storage,
		freeDesc[2].storage
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
			const auto preparedFiberStackAddress = currentAddress + stackSize - sizeof(Fiber);

			::new (reinterpret_cast<void*>(preparedFiberStackAddress)) Fiber(stackLimit, stackType, freeSetIter);
			ptrdiff_t offset = preparedFiberStackAddress - reinterpret_cast<uintptr_t>(stackPoolPtr);
			ASSERT((offset & 0xFFFFFFFF00000000) == 0ll);
			*freeSetIter = static_cast<uint32_t>(offset);
		}
	}
}

// clang-format on

FiberBased::~FiberBased() {
	const auto result = ::VirtualFree(stackPoolPtr, 0, MEM_RELEASE);
	ASSERT(result);
	allocator.deallocate(reinterpret_cast<uint8_t*>(queueBuffer), 1);
}

}
