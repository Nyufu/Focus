#pragma once

#include <Core/Concurrency/TaskParameters.hpp>
#include <Core/Concurrency/_impl/Fiber.hpp>

namespace Focus::Concurrency::_impl::Scheduler {

FiberHandle GetEmptyFiber(StackSize stackSize) noexcept;
void ScheduleFiber(FiberHandle fiberHandle, Priority priority) noexcept;
void ReleaseFiber(FiberHandle fiberHandle) noexcept;

}
