#pragma once

#include <Macros.h>
#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/_impl/Fiber.hpp>

#include <assert.hxx>
#include <algorithm.hxx>
#include <enum_helper.hxx>
#include <Macros.h>
#include <Platform.h>
#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/TaskParameters.hpp>
#include <Core/Memory/Allocator/Heap.hpp>

#include <Core/Memory/Allocator/Heap.hpp>
#include <Platform/WinNTEx.h>

namespace Focus::Concurrency::_impl::Scheduler {

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

class FiberBased {
	friend struct Thread;

private:
	using QueueEntry = Fiber*;
	using FreeSetEntry = uint32_t;

	struct FreeSetDescriptor {
		const unsigned __int64 size;
		union {
			FreeSetEntry* const storage;
			const uintptr_t ustorage;
		};

		FreeSetDescriptor(unsigned __int64 size, void* const memory)
			: size{ size }
			, ustorage{ STDEX align(reinterpret_cast<uintptr_t>(memory), platform::cacheline_size, STDEX align_way::up) } {
		}

		FreeSetDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: size{ size }
			, ustorage{ STDEX align(desc.ustorage + desc.size, platform::cacheline_size, STDEX align_way::up) } {
		}
	};

	struct QueueDescriptor {
		const unsigned __int64 capacityMask;
		union {
			const unsigned __int64 redBlackBit;
			const unsigned __int64 maxSize;
		};
		QueueEntry* const storage;

		QueueDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: capacityMask{ size - 1 }
			, maxSize{ size }
			, storage{ reinterpret_cast<QueueEntry*>(STDEX align(desc.ustorage + desc.size, platform::cacheline_size, STDEX align_way::up)) } {
		}

		QueueDescriptor(const QueueDescriptor& desc)
			: capacityMask{ desc.capacityMask }
			, maxSize{ desc.maxSize }
			, storage{ reinterpret_cast<QueueEntry*>(
				  STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + (desc.maxSize * platform::register_size), platform::cacheline_size, STDEX align_way::up)) } {
		}
	};

	struct alignas(platform::cacheline_size) AlignedValue {
		unsigned __int64 value = 0;
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
	Fiber* GetReadyFiber(Priority priority);

private:
	//[[no_unique_address]]
	Memory::Allocator::Heap<uint8_t> allocator;

	STD vector<HANDLE> threadHandles;
	void* stackPoolPtr;
	void* const queueBuffer;

	STD array<FreeSetDescriptor, enum_count<StackSize>> freeDesc;
	STD array<QueueDescriptor, enum_count<Priority>> queueDesc;

	STD array<AlignedValue, enum_count<Priority>> preHead;
	STD array<AlignedValue, enum_count<Priority>> postHead;
	STD array<AlignedValue, enum_count<Priority>> tail;
};

WARNING_SCOPE_END

inline Fiber* FiberBased::GetEmptyFiber(StackSize stackSize) noexcept {
	const auto zero = _mm256_setzero_si256();
	const auto random = (reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) - 0x28) << 2;
	// const auto random = (reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) & 0xFFFFFFFFFFFFFFC0);

	auto& desc = freeDesc[STDEX to_num(stackSize)];
	const auto size = desc.size;
	ASSERT((size & 0x000000000000003f) == 0ull);
	const auto storage = reinterpret_cast<uintptr_t>(desc.storage);

	ptrdiff_t offset = random % size;
	const auto startPoint = STD assume_aligned<platform::cacheline_size>(reinterpret_cast<__m256i*>(storage + offset));
	for (auto it = startPoint;;) {
		const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi32(*it, zero));
		if (mask != 0xFFFFFFFF) {
			const auto targetOffset = STD countr_zero(~mask);
			const auto targetPtr = reinterpret_cast<FreeSetEntry volatile*>(reinterpret_cast<uintptr_t>(it) + targetOffset);
			const auto fiberOffset = InterlockedExchange(targetPtr, 0ul);
			if (fiberOffset == 0u)
				continue;

			return reinterpret_cast<Fiber*>(reinterpret_cast<uintptr_t>(stackPoolPtr) + fiberOffset);
		}

		offset = (offset + sizeof(__m256i)) % size;
		it = STD assume_aligned<sizeof(__m256i)>(reinterpret_cast<__m256i*>(storage + offset));
		if (startPoint == it) {
			return nullptr; // Put wait on address here.
		}
	}
}

inline void FiberBased::ReleaseFiber(Fiber* fiber) noexcept {
	ASSERT(fiber->freeSetPlace);
	ASSERT(!*fiber->freeSetPlace);
	*fiber->freeSetPlace = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fiber) - reinterpret_cast<uintptr_t>(stackPoolPtr));
	// Signal to address.
}

inline void FiberBased::ScheduleFiber([[maybe_unused]] Fiber* fiber, [[maybe_unused]] Priority priority) {
}

inline Fiber* FiberBased::GetReadyFiber(Priority priority) {
}

}
