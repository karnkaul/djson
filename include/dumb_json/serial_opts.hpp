#pragma once
#include <cstdint>
#include <string>

namespace dj {
///
/// \brief Serialisation options
///
struct serial_opts_t {
	///
	/// \brief Set true to sort all keys (costs overhead)
	///
	bool sort_keys = false;
	///
	/// \brief Set false to collapse all whitespace
	///
	bool pretty = true;
	///
	/// \brief Set non-zero to use spaces instead of tabs
	///
	std::uint8_t space_indent = 0;
	///
	/// \brief String used for newlines
	///
	std::string_view newline = "\n";
};
} // namespace dj
