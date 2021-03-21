#include "Interface.hpp"
#include "FiberBased.hpp"
#include <Macros.h>

MSVC_WARNING_DISABLE(4075)	   // supress an init_seg warning which works in this case
#pragma init_seg(".CRT$XCU-Z") // The Scheduler::instance will be in instantiated last

namespace Focus::Concurrency::_impl::Scheduler {

static FiberBased instance;

Fiber* GetEmptyFiber(StackSize stackSize) noexcept {
	return instance.GetEmptyFiber(stackSize);
}

void ScheduleFiber(Fiber* fiber, Priority priority) noexcept {
	instance.ScheduleFiber(fiber, priority);
}

void ReleaseFiber(Fiber* fiber) noexcept {
	instance.ReleaseFiber(fiber);
}

}
