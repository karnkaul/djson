#include <iostream>
#include <mutex>
#include <dumb_json/error_handler.hpp>
#include <kt/str_format/str_format.hpp>

namespace dj {
namespace {
std::mutex g_mutex;
log_error_t g_log_error = &error_handler_t::default_callback;
} // namespace

void error_handler_t::default_callback(error_location_t const& error) {
	auto const str = kt::format_str("[djson] Error: {} [l: {} | c: {}]", error.message, error.line, error.column);
	if constexpr (output == output_t::std_out) {
		std::cout << str << std::endl;
	} else {
		std::cerr << str << std::endl;
	}
}

void error_handler_t::set_callback(log_error_t callback) const {
	std::scoped_lock<std::mutex> lock(g_mutex);
	g_log_error = callback;
}

void error_handler_t::operator()(error_location_t const& error) const {
	std::scoped_lock<std::mutex> lock(g_mutex);
	if (g_log_error) { g_log_error(error); }
	if constexpr (action == action_t::throw_ex || action == action_t::unwind) {
		throw parse_error_t(error);
	} else if constexpr (action == action_t::terminate) {
		std::terminate();
	}
}
} // namespace dj
