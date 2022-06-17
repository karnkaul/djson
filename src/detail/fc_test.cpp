#include <charconv>

namespace {
[[maybe_unused]] void fc_test() {
	char str[] = "3.14f";
	auto f = float{};
	std::from_chars(str, str + sizeof(str), f);
}
} // namespace
