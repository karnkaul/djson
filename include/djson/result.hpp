#pragma once
#include <string>

namespace dj {
///
/// \brief Syntax / semantic error in a JSON string
///
struct parse_result {
	///
	/// \brief Unexpected token (or "(empty)" if missing)
	///
	std::string token{};
	std::size_t line{};
	std::size_t column{};

	///
	/// \brief Check if result does not contain an error
	///
	explicit operator bool() const { return line == 0; }
};

///
/// \brief Filesystem or parse error
///
struct io_result : parse_result {
	///
	/// \brief Path to file which caused the error
	///
	std::string file_path{};
	std::string file_contents{};

	///
	/// \brief Check if result does not contain an error
	///
	explicit operator bool() const { return file_path.empty() && line == 0; }
};
} // namespace dj
