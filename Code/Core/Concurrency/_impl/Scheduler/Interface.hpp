#pragma once

#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct Fiber;

namespace Scheduler {

Fiber* GetEmptyFiber(StackSize stackSize);
void ScheduleFiber(Fiber* fiber, Priority priority);
void ReleaseFiber(Fiber* fiber);


}
}
