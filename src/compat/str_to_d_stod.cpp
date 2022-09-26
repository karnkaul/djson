#include <string>

namespace dj::detail {
bool str_to_d(std::string_view const str, double& out) {
	try {
		out = std::stod(std::string{str});
		return true;
	} catch (std::exception const&) { return false; }
}
} // namespace dj::detail
