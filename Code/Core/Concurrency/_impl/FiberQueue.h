#pragma once

#include <enum_helper.hxx>
#include <Macros.h>
#include <Platform.h>
#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/TaskParameters.hpp>

namespace Focus::Concurrency::_impl {

struct Fiber;

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

struct alignas(platform::cacheline_size) FiberQueue {
	ONLY_CONSTRUCT(FiberQueue);

public:
	using DataType = Fiber*;

protected:
	using DataTypePtr = DataType*;

public:
	FiberQueue(const Config::TaskQueue& initDesc);
	~FiberQueue();

	void Enqueue(StackSize stackSize, DataType value);
	void Enqueue(Priority priority, DataType value);

	bool Dequeue(StackSize stackSize, DataType& value);
	bool Dequeue(Priority priority, DataType& value);

protected:
	void Enqueue(uint8_t queueIndex, DataType value);
	bool Dequeue(uint8_t queueIndex, DataType& value);
};

WARNING_SCOPE_END

inline FiberQueue::FiberQueue(const Config::TaskQueue&) {
}

inline FiberQueue::~FiberQueue() {
}

inline void FiberQueue::Enqueue(StackSize stackSize, DataType value) {
	Enqueue(STDEX to_num(stackSize), value);
}
inline void FiberQueue::Enqueue(Priority priority, DataType value) {
	Enqueue(STDEX to_num(priority), value);
}
inline bool FiberQueue::Dequeue(StackSize stackSize, DataType& value) {
	return Dequeue(STDEX to_num(stackSize), value);
}
inline bool FiberQueue::Dequeue(Priority priority, DataType& value) {
	return Dequeue(STDEX to_num(priority), value);
}

inline void FiberQueue::Enqueue([[maybe_unused]] uint8_t queueIndex, [[maybe_unused]] DataType value) {
}
inline bool FiberQueue::Dequeue([[maybe_unused]] uint8_t queueIndex, [[maybe_unused]] DataType& value) {
	return false;
}

}