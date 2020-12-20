#pragma once

#include <types.hxx>
#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct Fiber {
	constexpr Fiber(register_t stackLimit_, StackSize stackSize_) noexcept
		: stackLimit{ stackLimit_ }
		, stackSize{ stackSize_ }
		, stackPointer{ 0 } {
	}

	// The value of this is the stack limit,
	// the address of this is the stack bottom.
	const register_t stackLimit;

	const StackSize stackSize;

	register_t stackPointer;
};

}
