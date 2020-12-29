#pragma once

#include <types.hxx>
#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct Fiber {
	constexpr Fiber(register_t stackLimit, StackSize stackSize, uint32_t* freeSetPlace) noexcept
		: stackLimit{ stackLimit }
		, stackSize{ stackSize }
		, freeSetPlace{ freeSetPlace }
		, stackPointer{ 0 } {
	}

	// The value of this is the stack limit,
	// the address of this is the stack bottom.
	const register_t stackLimit;

	const StackSize stackSize;

	uint32_t* const freeSetPlace;

	register_t stackPointer;
};

}
