#pragma once

namespace Focus::Concurrency {

template <class Callable, class... Args>
[[nodiscard]] auto CreateTask(Callable&& callable, Args&&... args) {
	return Task<int>{};
}

template <class Callable, class... Args>
void Async([[maybe_unused]] Callable&& callable, [[maybe_unused]] Args&&... args) {
}

}
