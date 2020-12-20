#pragma once

namespace stdex {

template <class Enum>
constexpr auto to_num(Enum value) noexcept -> STD enable_if_t<STD is_enum_v<Enum>, STD underlying_type_t<Enum>> {
	return static_cast<STD underlying_type_t<Enum>>(value);
}

}