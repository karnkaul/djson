#include <charconv>
#include <string_view>

namespace dj::detail {
bool str_to_d(std::string_view const str, double& out) {
	auto const [_, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
	return ec == std::errc{};
}
} // namespace dj::detail
