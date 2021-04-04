#pragma once

#include "TaskParameters.hpp"

namespace Focus::Concurrency {

template <class Ty>
class Task {
public:
	using type = Ty;

public:
	Task() noexcept = default;
	~Task() = default;
};

}

#if defined(__INTELLISENSE__)

#include "_impl/CreateTaskHelper_EDG.inl"

#else // vvv real implementation vvv

#include "_impl/CreateTask.inl"

#endif
