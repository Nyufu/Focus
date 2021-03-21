#pragma once

#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct Fiber;

namespace Scheduler {

Fiber* GetEmptyFiber(StackSize stackSize) noexcept;
void ScheduleFiber(Fiber* fiber, Priority priority) noexcept;
void ReleaseFiber(Fiber* fiber) noexcept;

}
}
