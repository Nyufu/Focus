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
	using FreeSetEntry = uint32_t;

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

	struct AlignedValue {
		alignas(platform::cacheline_size) STD atomic_uint64_t value;
	};

public:
	using FiberPtr = Fiber*;

public:
	ONLY_CONSTRUCT(FiberBased);
	FiberBased();
	~FiberBased();

	FiberPtr GetEmptyFiber(StackSize stackSize) noexcept;
	void ReleaseFiber(FiberPtr fiber) noexcept;
	void ScheduleFiber(FiberPtr fiber, Priority priority);
	FiberPtr GetReadyFiber(Priority priority) noexcept;

private:
	FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes);
	FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, long numberOfThreads);
	FiberBased(size_t largeSizeInBytes, size_t mediumSizeInBytes, size_t smallSizeInBytes, size_t freeSetSizeInBytes, size_t queueSize, size_t queueSizeInBytes, long numberOfThreads);

	void Init(size_t freeSetSizeInBytes, size_t queueSizeInBytes, long numberOfThreads);

	void Signal() noexcept;
	uint64_t WaitForTail(const STD atomic_uint64_t& tail, const FiberPtr& targetPosition, uintptr_t currentValue) noexcept;

private:
	//[[no_unique_address]]
	Memory::Allocator::Heap<uint8_t> allocator;

	STD vector<HANDLE> threadHandles;
	void* const queueBuffer;

	alignas(platform::cacheline_size) void* stackPoolPtr;

	const STD array<uint64_t, enum_count<StackSize>> freePoolSizesInBytes;
	const STD array<FreeSetEntry*, enum_count<StackSize>> freePools;

	alignas(platform::cacheline_size) QueueDescriptor queueDesc;
	const STD array<FiberPtr*, enum_count<Priority>> queues;

	STD array<AlignedValue, enum_count<Priority>> heads;
	STD array<AlignedValue, enum_count<Priority>> tails;

	alignas(platform::cacheline_size) STD atomic_int64_t waitCounter;

private:
	static FiberPtr* GetValue(const STD array<FiberPtr*, enum_count<Priority>>& array, Priority priority) noexcept {
		const auto offset = STDEX to_num(priority);
		return *reinterpret_cast<FiberPtr* const*>(reinterpret_cast<const uint8_t*>(array.data()) + offset);
	}

	static STD atomic_uint64_t& GetValue(STD array<AlignedValue, enum_count<Priority>>& array, Priority priority) noexcept {
		const auto offset = STDEX to_num(priority) * platform::registers_in_cacheline;
		return reinterpret_cast<AlignedValue*>(reinterpret_cast<uint8_t*>(array.data()) + offset)->value;
	}
};

WARNING_SCOPE_END

inline FiberBased::FiberPtr FiberBased::GetEmptyFiber(StackSize stackSize) noexcept {
	//_mm_prefetch(reinterpret_cast<const char*>(STD addressof(freeDescs)), _MM_HINT_NTA);

	const auto zero = _mm256_setzero_si256();
	const auto random = _readgsbase_u64();

	const auto index = STDEX to_num(stackSize);
	const auto size = freePoolSizesInBytes[index];
	ASSERT((size & 0x000000000000003f) == 0ull);
	const auto storage = reinterpret_cast<uintptr_t>(freePools[index]);

	ptrdiff_t offset = random % size;
	const auto startPoint = STD assume_aligned<platform::cacheline_size>(reinterpret_cast<__m256i*>(storage + offset));
	ASSERT(startPoint);
	for (auto it = startPoint;;) {
		const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi32(*it, zero));
		if (mask != 0xFFFFFFFF) {
			const auto targetOffset = STD countr_one(mask);
			const auto targetPtr = reinterpret_cast<FreeSetEntry volatile*>(reinterpret_cast<uintptr_t>(it) + targetOffset);
			const auto fiberOffset = _InterlockedExchange(targetPtr, 0ul);
			if (fiberOffset == 0u)
				continue;

			return reinterpret_cast<FiberPtr>(reinterpret_cast<uintptr_t>(stackPoolPtr) + fiberOffset);
		}

		const auto incremented = offset + sizeof(__m256i);
		offset = incremented >= size ? incremented - size : incremented;
		it = STD assume_aligned<sizeof(__m256i)>(reinterpret_cast<__m256i*>(storage + offset));
		if (startPoint == it) {
			return nullptr; // There is no free fiber, need to finish one from the queue.
		}
	}
}

inline void FiberBased::ReleaseFiber(FiberPtr fiber) noexcept {
	auto impl = static_cast<FiberImpl*>(fiber);
	ASSERT(impl->freeSetPlace);
	ASSERT(!*impl->freeSetPlace);
	*impl->freeSetPlace = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fiber) - reinterpret_cast<uintptr_t>(stackPoolPtr));
}

inline void FiberBased::ScheduleFiber(FiberPtr fiber, Priority priority) {
	_mm_prefetch(reinterpret_cast<const char*>(STD addressof(queueDesc)), _MM_HINT_NTA);

	auto& head = GetValue(heads, priority);
	const auto targetIndex = head.fetch_add(1, STD memory_order::relaxed);
	const auto queue = GetValue(queues, priority);
	const auto mask = queueDesc.GetMask();
	const auto redBlackMask = queueDesc.GetRedBlackMask();
	const uint8_t redBlackBit = targetIndex & redBlackMask ? 1 : 0;

	auto ptr = STD assume_aligned<256>(fiber);
	queue[targetIndex & mask] = reinterpret_cast<FiberPtr>(reinterpret_cast<uintptr_t>(ptr) | redBlackBit);

	if (static_cast<uint8_t>(waitCounter.load(STD memory_order::relaxed))) [[unlikely]]
		Signal();
}

inline FiberBased::FiberPtr FiberBased::GetReadyFiber(Priority priority) noexcept {
	auto& tail = GetValue(tails, priority);
	const auto queue = GetValue(queues, priority);
	const auto mask = queueDesc.GetMask();
	const auto redBlackMask = queueDesc.GetRedBlackMask();

	for (auto currentTail = tail.load(STD memory_order::relaxed);;) {
		auto& valueRef = queue[currentTail & mask];
		const auto currentValue = reinterpret_cast<uintptr_t>(valueRef);
		const auto redBlackBit = static_cast<uint8_t>(currentValue);
		const uint8_t target = currentTail & redBlackMask ? 1 : 0;

		if (target ^ redBlackBit) [[unlikely]] {
			currentTail = WaitForTail(tail, valueRef, currentValue);
			continue;
		}

		if (tail.compare_exchange_strong(currentTail, currentTail + 1))
			return reinterpret_cast<FiberPtr>(currentValue & 0xFFFFFFFFFFFFFF00);
	}
}
}
