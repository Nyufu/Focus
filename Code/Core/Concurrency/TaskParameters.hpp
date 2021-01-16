#pragma once

#include <types.hxx>
#include <enum_helper.hxx>

namespace Focus::Concurrency {

inline namespace TaskParameter {

enum class StackSize : register_t
{
	Medium = 0,
	Large,
	Small
};

enum class Priority : register_t
{
	Normal = 0,
	High,
	Low
};

}

namespace Default {

inline constexpr auto Priority = TaskParameter::Priority::Normal;
inline constexpr auto StackSize = TaskParameter::StackSize::Medium;

}

}

template <>
INLINE constexpr size_t enum_count<Focus::Concurrency::TaskParameter::StackSize> = 3;

template <>
INLINE constexpr size_t enum_count<Focus::Concurrency::TaskParameter::Priority> = 3;