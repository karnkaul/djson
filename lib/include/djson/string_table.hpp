#pragma once
#include <string>
#include <unordered_map>

namespace dj {
/// \brief Heterogeneous string hasher.
struct StringHash : std::hash<std::string_view> {
	using is_transparent = void;
};

/// \brief Heterogeneous string map.
template <typename Value>
using StringTable = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;
} // namespace dj
