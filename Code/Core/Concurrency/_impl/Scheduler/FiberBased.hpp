#pragma once

#include <assert.hxx>
#include <algorithm.hxx>
#include <enum_helper.hxx>
#include <Macros.h>
#include <Platform.h>
#include <Core/Concurrency/TaskParameters.hpp>
#include <Core/Concurrency/_impl/Fiber.hpp>
#include <Core/Memory/Allocator/Heap.hpp>

namespace Focus::Concurrency::_impl::Scheduler {

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

class FiberBased {
	friend struct Thread;

private:
	using QueueEntry = Fiber*;
	using FreeSetEntry = uint32_t;

	struct FreeSetDescriptor {
		const uint64_t size;
		union {
			FreeSetEntry* const storage;
			const uintptr_t ustorage;
		};

		FreeSetDescriptor(uint64_t size, void* const memory) noexcept
			: size{ size }
			, ustorage{ STDEX align(reinterpret_cast<uintptr_t>(memory), platform::cacheline_size, STDEX align_way::up) } {
		}

		FreeSetDescriptor(uint64_t size, const FreeSetDescriptor& desc) noexcept
			: size{ size }
			, ustorage{ STDEX align(desc.ustorage + desc.size, platform::cacheline_size, STDEX align_way::up) } {
		}
	};

	struct QueueDescriptor {
		const uint64_t capacityMask;

		QueueDescriptor(uint64_t size) noexcept
			: capacityMask{ size - 1 } {
		}

		uint64_t GetMask() const noexcept {
			return capacityMask;
		}

		uint64_t GetSize() const noexcept {
			return capacityMask + 1;
		}

		uint64_t GetRedBlackMask() const noexcept {
			return GetSize();
		}
	};

	struct QueueControl {
		alignas(platform::cacheline_size) STD atomic_uint64_t head;
		alignas(platform::cacheline_size) STD atomic_uint64_t tail;
		alignas(platform::cacheline_size) STD atomic_int64_t waitCounter;

		void Signal() noexcept;
		uint64_t WaitForTail(const QueueEntry& targetPosition, const uint64_t redBlackBit) noexcept;
	};

public:
	ONLY_CONSTRUCT(FiberBased);
	FiberBased();
	FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes);
	FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, long numberOfThreads);
	~FiberBased();

	Fiber* GetEmptyFiber(StackSize stackSize) noexcept;
	void ReleaseFiber(Fiber* fiber) noexcept;
	void ScheduleFiber(Fiber* fiber, Priority priority);
	Fiber* GetReadyFiber(Priority priority) noexcept;

private:
	void Init(size_t freeSetSizeInBytes, long numberOfThreads);

private:
	//[[no_unique_address]]
	Memory::Allocator::Heap<uint8_t> allocator;

	STD vector<HANDLE> threadHandles;
	void* const queueBuffer;

	alignas(platform::cacheline_size) void* stackPoolPtr;

	STD array<FreeSetDescriptor, enum_count<StackSize>> freeDescs;

	alignas(platform::cacheline_size) QueueDescriptor queueDesc;
	const STD array<QueueEntry*, enum_count<Priority>> queues;

	STD array<QueueControl, enum_count<Priority>> queueControls;
};

WARNING_SCOPE_END

inline Fiber* FiberBased::GetEmptyFiber(StackSize stackSize) noexcept {
	//_mm_prefetch(reinterpret_cast<const char*>(STD addressof(freeDescs)), _MM_HINT_NTA);

	const auto zero = _mm256_setzero_si256();
	// const auto random = (reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) - 0x28) << 2;
	const auto random = (reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) & 0xFFFFFFFFFFFFFFC0);

	auto& desc = freeDescs[STDEX to_num(stackSize)];
	const auto size = desc.size;
	ASSERT((size & 0x000000000000003f) == 0ull);
	const auto storage = reinterpret_cast<uintptr_t>(desc.storage);

	ptrdiff_t offset = random % size;
	const auto startPoint = STD assume_aligned<platform::cacheline_size>(reinterpret_cast<__m256i*>(storage + offset));
	ASSERT(startPoint);
	for (auto it = startPoint;;) {
		const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi32(*it, zero));
		if (mask != 0xFFFFFFFF) {
			const auto targetOffset = STD countr_zero(~mask);
			const auto targetPtr = reinterpret_cast<FreeSetEntry volatile*>(reinterpret_cast<uintptr_t>(it) + targetOffset);
			const auto fiberOffset = _InterlockedExchange(targetPtr, 0ul);
			if (fiberOffset == 0u)
				continue;

			return reinterpret_cast<Fiber*>(reinterpret_cast<uintptr_t>(stackPoolPtr) + fiberOffset);
		}

		offset = (offset + sizeof(__m256i)) % size;
		it = STD assume_aligned<sizeof(__m256i)>(reinterpret_cast<__m256i*>(storage + offset));
		if (startPoint == it) {
			return nullptr; // There is no free fiber, need to finish one from the queue.
		}
	}
}

inline void FiberBased::ReleaseFiber(Fiber* fiber) noexcept {
	ASSERT(fiber->freeSetPlace);
	ASSERT(!*fiber->freeSetPlace);
	*fiber->freeSetPlace = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fiber) - reinterpret_cast<uintptr_t>(stackPoolPtr));
}

inline void FiberBased::ScheduleFiber(Fiber* fiber, Priority priority) {
	_mm_prefetch(reinterpret_cast<const char*>(STD addressof(queueDesc)), _MM_HINT_NTA);

	const auto prioIndex = STDEX to_num(priority);
	auto& queueControl = queueControls[prioIndex];

	const auto targetIndex = queueControl.head.fetch_add(1, STD memory_order::relaxed);

	const auto mask = queueDesc.GetMask();
	const auto redBlackMask = queueDesc.GetRedBlackMask();
	const auto queue = queues[prioIndex];

	// ASSERT(STD has_single_bit(redBlackMask));

	queue[targetIndex & mask] = reinterpret_cast<Fiber*>(reinterpret_cast<uintptr_t>(fiber) | size_t((targetIndex & redBlackMask) != 0));

	if (queueControl.waitCounter) [[unlikely]]
		queueControl.Signal();
}

inline Fiber* FiberBased::GetReadyFiber(Priority priority) noexcept {
	//_mm_prefetch(reinterpret_cast<const char*>(STD addressof(queueDesc)), _MM_HINT_NTA);

	const auto prioIndex = STDEX to_num(priority);
	auto& queueControl = queueControls[prioIndex];
	const auto queue = queues[prioIndex];
	const auto mask = queueDesc.GetMask();
	const auto redBlackMask = queueDesc.GetRedBlackMask();

	// ASSERT(STD has_single_bit(redBlackMask));

	for (auto currentTail = queueControl.tail.load(STD memory_order::relaxed);;) {
		auto localValue = reinterpret_cast<uintptr_t>(queue[currentTail & mask]);

		if ((localValue & 1) != (currentTail & redBlackMask ? 1 : 0)) [[unlikely]] {
			currentTail = queueControl.WaitForTail(queue[currentTail & mask], localValue & 1);
			continue;
		}

		if (queueControl.tail.compare_exchange_strong(currentTail, currentTail + 1))
			return reinterpret_cast<Fiber*>(localValue & ~1ui64);
	}
}

}
