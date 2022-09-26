#include <memory>

namespace dj::detail {
std::unique_ptr<char[]> make_char_buf(std::size_t size) { return std::make_unique_for_overwrite<char[]>(size); }
} // namespace dj::detail
