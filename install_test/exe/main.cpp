#include <djson/build_version.hpp>
#include <print>

auto main() -> int { std::println("djson version: {}", dj::build_version_v); }
