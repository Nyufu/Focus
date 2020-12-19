#pragma once

using register_t = uintptr_t;

constexpr size_t operator"" _KB(size_t value) {
	return value * 1024Ui64;
}
