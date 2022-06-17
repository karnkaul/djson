#pragma once
#include <charconv>
#include <concepts>

#if !defined(DJSON_HAS_FROM_CHARS_FLOAT)
#include <cstdlib>
#endif

namespace dj::detail {
template <typename T>
std::from_chars_result from_chars(char const* first, char const* last, T& out) {
#if !defined(DJSON_HAS_FROM_CHARS_FLOAT)
	if constexpr (std::floating_point<T>) {
		char* next;
		out = std::strtof(first, &next);
		return {next, next == first ? std::errc::invalid_argument : std::errc{}};
	} else {
		return std::from_chars(first, last, out);
	}
#else
	return std::from_chars(first, last, out);
#endif
}
} // namespace dj::detail
