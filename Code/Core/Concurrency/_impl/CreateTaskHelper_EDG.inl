#pragma once

#include "CommonHeaders.hpp"

namespace Focus::Concurrency {
namespace _impl {

// Only for IntelliSense speed improvement

template <class C, class... Ts>
STD invoke_result_t<C, Ts...> CTH(C&&, Ts&&...);

#define CREATE_TASK_HELPER_WITH_PARAMETER(ParameterType)                                                                                               \
	template <class C, class T0, class... Ts>                                                                                                          \
	STD invoke_result_t<C, T0> CTH(C&&, T0&&, ParameterType, Ts&&...);                                                                                 \
																																					   \
	template <class C, class T0, class T1, class... Ts>                                                                                                \
	STD invoke_result_t<C, T0, T1> CTH(C&&, T0&&, T1&&, ParameterType, Ts&&...);                                                                       \
																																					   \
	template <class C, class T0, class T1, class T2, class... Ts>                                                                                      \
	STD invoke_result_t<C, T0, T1, T2> CTH(C&&, T0&&, T1&&, T2&&, ParameterType, Ts&&...);                                                             \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class... Ts>                                                                            \
	STD invoke_result_t<C, T0, T1, T2, T3> CTH(C&&, T0&&, T1&&, T2&&, T3&&, ParameterType, Ts&&...);                                                   \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class... Ts>                                                                  \
	STD invoke_result_t<C, T0, T1, T2, T3, T4> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, ParameterType, Ts&&...);                                         \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class T5, class... Ts>                                                        \
	STD invoke_result_t<C, T0, T1, T2, T3, T4, T5> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, T5&&, ParameterType, Ts&&...);                               \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class... Ts>                                              \
	STD invoke_result_t<C, T0, T1, T2, T3, T4, T5, T6> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, T5&&, T6&&, ParameterType, Ts&&...);                     \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class... Ts>                                    \
	STD invoke_result_t<C, T0, T1, T2, T3, T4, T5, T6, T7> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, T5&&, T6&&, T7&&, ParameterType, Ts&&...);           \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class... Ts>                          \
	STD invoke_result_t<C, T0, T1, T2, T3, T4, T5, T6, T7, T8> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, T5&&, T6&&, T7&&, T8&&, ParameterType, Ts&&...); \
																																					   \
	template <class C, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... Ts>                \
	STD invoke_result_t<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> CTH(C&&, T0&&, T1&&, T2&&, T3&&, T4&&, T5&&, T6&&, T7&&, T8&&, T9&&, ParameterType, Ts&&...);

CREATE_TASK_HELPER_WITH_PARAMETER(TaskParameter::Priority)
CREATE_TASK_HELPER_WITH_PARAMETER(TaskParameter::StackSize)

}

template <class Callable, class... Args>
[[nodiscard]] Task<decltype(_impl::CTH(STD declval<Callable>(), STD declval<Args>()...))> CreateTask(Callable&&, Args&&...);

template <class Callable, class... Args>
void Async(Callable&&, Args&&...);

}
