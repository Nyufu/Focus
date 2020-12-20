#pragma once

namespace Focus::Concurrency {

inline namespace TaskParameter {

enum class Priority : uint8_t
{
	Normal = 0,
	High,
	Low
};

enum class StackSize : uint8_t
{
	Medium = 3,
	Large,
	Small
};

}

namespace Default {

inline constexpr auto Priority = TaskParameter::Priority::Normal;
inline constexpr auto StackSize = TaskParameter::StackSize::Medium;

}

}
