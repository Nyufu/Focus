#pragma once

namespace Focus::Concurrency {

inline namespace TaskParameter {

enum class StackSize : uint8_t
{
	Medium = 0,
	Large,
	Small
};

enum class Priority : uint8_t
{
	Normal = 3,
	High,
	Low
};

}

namespace Default {

inline constexpr auto Priority = TaskParameter::Priority::Normal;
inline constexpr auto StackSize = TaskParameter::StackSize::Medium;

}

}
