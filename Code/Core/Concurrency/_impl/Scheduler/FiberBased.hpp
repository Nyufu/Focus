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

namespace Focus::Concurrency::_impl::Scheduler {

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

class FiberBased {
	friend struct Thread;

private:
	struct FreeSetDescriptor {
		const unsigned __int64 size;
		Fiber** const storage = nullptr;

		FreeSetDescriptor(unsigned __int64 size, void* const memory)
			: size{ size * sizeof(Fiber*) }
			, storage{ reinterpret_cast<Fiber**>(STDEX align(reinterpret_cast<uintptr_t>(memory), platform::cacheline_size, STDEX align_way::up)) } {
		}

		FreeSetDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: size{ size * sizeof(Fiber*) }
			, storage{ reinterpret_cast<Fiber**>(STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + desc.size, platform::cacheline_size, STDEX align_way::up)) } {
		}
	};

	struct QueueDescriptor {
		const unsigned __int64 capacityMask;
		union {
			const unsigned __int64 redBlackBit;
			const unsigned __int64 maxSize;
		};
		Fiber** const storage = nullptr;

		QueueDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: capacityMask{ size - 1 }
			, maxSize{ size }
			, storage{ reinterpret_cast<Fiber**>(STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + desc.size, platform::cacheline_size, STDEX align_way::up)) } {
		}

		QueueDescriptor(const QueueDescriptor& desc)
			: capacityMask{ desc.capacityMask }
			, maxSize{ desc.maxSize }
			, storage{ reinterpret_cast<Fiber**>(
				  STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + (desc.maxSize * platform::register_size), platform::cacheline_size, STDEX align_way::up)) } {
		}
	};

	struct alignas(platform::cacheline_size) AlignedValue {
		unsigned __int64 value;
	};

public:
	ONLY_CONSTRUCT(FiberBased);
	FiberBased();
	FiberBased(size_t freeSetSize);
	~FiberBased();

	Fiber* GetEmptyFiber(StackSize stackSize);
	Fiber* GetReadyFiber(Priority priority);
	void ScheduleFiber(Fiber* fiber, Priority priority);
	void ReleaseFiber(Fiber* fiber);

private:
	STD vector<HANDLE> threadHandles;
	void* stackPoolPtr;
	void* const queueBuffer;

	STD array<FreeSetDescriptor, enum_count<StackSize>> freeDesc;
	STD array<QueueDescriptor, enum_count<Priority>> queueDesc;

	STD array<AlignedValue, enum_count<Priority>> head;
	STD array<AlignedValue, enum_count<Priority>> tail;
};

WARNING_SCOPE_END

inline Fiber* FiberBased::GetEmptyFiber(StackSize stackSize) {
	// for (;;) {
	//	auto fiber = queue.Acquire(stackSize);
	//
	//	if (fiber)
	//		return fiber;
	//}
	return nullptr;
}

inline void FiberBased::ScheduleFiber([[maybe_unused]] Fiber* fiber, [[maybe_unused]] Priority priority) {
}

inline void FiberBased::ReleaseFiber(Fiber* fiber) {
	ASSERT(!fiber->freeSetPlace);
	*fiber->freeSetPlace = fiber;
}


}
