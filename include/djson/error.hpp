#pragma once
#include <string>

namespace dj {
struct parse_error {
	std::string token{};
	std::size_t line{};
	std::size_t column{};

	explicit operator bool() const { return line > 0; }

	template <typename OstreamT>
	friend OstreamT& operator<<(OstreamT& out, parse_error const& err) {
		if (err) { out << "[djson] Unexpected token [" << err.token << "] at line " << err.line << " col " << err.column; }
		return out;
	}
};

struct io_error : parse_error {
	std::string file_path{};
	std::string file_contents{};

	explicit operator bool() const { return !file_path.empty() || line > 0; }

	template <typename OstreamT>
	friend OstreamT& operator<<(OstreamT& out, io_error const& err) {
		if (!err.file_path.empty()) {
			out << "[djson] Failed to read file contents [" << err.file_path << "]";
		} else if (err.line > 0) {
			out << static_cast<parse_error>(err);
		}
		return out;
	}
};
} // namespace dj
