#pragma once
#include <stdexcept>
#include <string>

#if !defined(DJSON_PARSE_ERROR_ACTION)
#define DJSON_PARSE_ERROR_ACTION 0
#endif
#if !defined(DJSON_PARSE_ERROR_OUTPUT)
#define DJSON_PARSE_ERROR_OUTPUT 0
#endif

namespace dj {
///
/// \brief Diagnostic report type
///
struct error_location_t {
	std::string_view message;
	std::size_t line = 0;
	std::size_t column = 0;
};

///
/// \brief Exception type for parse errors
///
struct parse_error_t : std::runtime_error {
	error_location_t error;

	parse_error_t(error_location_t const& error) : std::runtime_error(error.message.data()), error(error) {}
};

///
/// \brief Exception type for node errors
///
struct node_error : std::runtime_error {
	std::string key;

	node_error(std::string key) : std::runtime_error(("djson: " + key + " not present").data()), key(std::move(key)) {}
};

///
/// \brief Error log callback
///
using log_error_t = void (*)(error_location_t const&);

struct error_handler_t {
	enum class action_t { unwind, throw_ex, terminate, ignore };
	enum class output_t { std_err, std_out };

	static constexpr action_t action = static_cast<action_t>(DJSON_PARSE_ERROR_ACTION);
	static constexpr output_t output = static_cast<output_t>(DJSON_PARSE_ERROR_OUTPUT);

	static void default_callback(error_location_t const& error);

	void operator()(error_location_t const& error) const;
	void set_callback(log_error_t callback) const;
};

inline error_handler_t g_error;
} // namespace dj
