#pragma once
#include <djson/json.hpp>
#include <djson/string_table.hpp>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace dj::detail {
namespace literal {
struct Bool {
	bool value{};
};

struct Number {
	using Payload = std::variant<double, std::uint64_t, std::int64_t>;

	Payload payload{};
};

struct String {
	std::string text{};
};
} // namespace literal

struct Array {
	std::vector<dj::Json> members{};
};

struct Object {
	StringTable<dj::Json> members{};
};

struct Value {
	using Payload = std::variant<literal::Bool, literal::Number, literal::String, Array, Object>;

	template <typename T>
	auto morph() -> T& {
		auto* ret = std::get_if<T>(&payload);
		if (!ret) { ret = &payload.emplace<T>(); }
		return *ret;
	}

	Payload payload{};
};
} // namespace dj::detail
