#pragma once

namespace dj::detail {
template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};
} // namespace dj::detail
