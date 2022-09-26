#pragma once
#include <memory>
#include <string_view>

namespace dj::detail {
bool str_to_d(std::string_view str, double& out);
std::unique_ptr<char[]> make_char_buf(std::size_t size);
} // namespace dj::detail
