#pragma once

namespace platform {

inline constexpr unsigned __int64 cacheline_size = STD hardware_destructive_interference_size;
inline constexpr unsigned __int64 cacheline_mask = cacheline_size - 1;

inline constexpr unsigned __int64 register_size = sizeof(void*);
inline constexpr unsigned __int64 registers_in_cacheline = cacheline_size / register_size;

inline constexpr unsigned __int64 default_align = register_size * 2;

}
