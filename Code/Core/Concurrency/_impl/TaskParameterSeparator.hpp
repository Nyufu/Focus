#pragma once

#include "CommonHeaders.hpp"

namespace Focus::Concurrency::_impl {

template<class...>
struct TaskParameterSeparator_impl;

template<typename... Types>
using TaskParameterSeparator = typename TaskParameterSeparator_impl<Types...>::type;

template<class Ty, class... Types>
struct TaskParameterSeparator_impl<Ty, Types...> {
	using type = STDEX append_type_list<Ty, TaskParameterSeparator<Types...>>;
};

template<>
struct TaskParameterSeparator_impl<> {
	using type = STDEX empty_type_list;
};

#define _TASK_PARAMETER_SEPARATOR_IMPL(C_OPT, TYPE, REF_OPT) \
template<class... Types> \
struct TaskParameterSeparator_impl<C_OPT TYPE REF_OPT, Types...> { \
	using type = STDEX empty_type_list; \
};

#define _TASK_PARAMETER_SEPARATOR_IMPL_SPECIALIZATION(C_OPT, REF_OPT) \
	_TASK_PARAMETER_SEPARATOR_IMPL(C_OPT, TaskParameter::Priority, REF_OPT) \
	_TASK_PARAMETER_SEPARATOR_IMPL(C_OPT, TaskParameter::StackSize, REF_OPT)

C_REF_SPECIALIZATION(_TASK_PARAMETER_SEPARATOR_IMPL_SPECIALIZATION)

#undef _TASK_PARAMETER_SEPARATOR_IMPL_SPECIALIZATION
#undef _TASK_PARAMETER_SEPARATOR_IMPL

}
