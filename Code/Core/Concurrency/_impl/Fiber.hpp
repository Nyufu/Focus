#pragma once

#include <types.hxx>
#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct FiberImpl {
	// The value of this is the stack limit,
	// the address of this is the stack bottom.
	const register_t stackLimit;

	const StackSize stackSize;

	uint32_t* const freeSetPlace;

	register_t stackPointer = 0;

	struct FiberHandleT {};
};

struct Fiber : FiberImpl, FiberImpl::FiberHandleT {};

using FiberHandle = FiberImpl::FiberHandleT*;

}