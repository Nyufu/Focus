#pragma once

#include <Macros.h>
#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/_impl/FiberQueue.h>

namespace Focus::Concurrency::_impl::Scheduler {

class FiberBased {
	friend struct Thread;

public:
	ONLY_CONSTRUCT(FiberBased);
	FiberBased();
	~FiberBased();

	Fiber* GetEmptyFiber(StackSize stackSize);
	void ScheduleFiber(Fiber* fiber, Priority priority);

private:
	FiberQueue queue;
	STD vector<HANDLE> threadHandles;
	void* stackPoolPtr;
};

inline Fiber* FiberBased::GetEmptyFiber(StackSize stackSize) {
	for (;;) {
		auto fiber = queue.Acquire(stackSize);

		if (fiber)
			return fiber;
	}
}

inline void FiberBased::ScheduleFiber([[maybe_unused]] Fiber* fiber, [[maybe_unused]] Priority priority) {
}

}
