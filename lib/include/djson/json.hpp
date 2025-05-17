#pragma once
#include <djson/error.hpp>
#include <djson/string_table.hpp>
#include <expected>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <utility>

namespace dj {
template <typename Type>
concept NumericT = std::integral<Type> || std::floating_point<Type>;

template <typename Type>
concept StringyT = std::convertible_to<Type, std::string_view>;

template <typename Type>
concept GettableT = std::same_as<Type, bool> || NumericT<Type> || StringyT<Type>;

template <typename Type>
concept SettableT = std::same_as<Type, std::nullptr_t> || GettableT<Type>;

class Json;
using Result = std::expected<Json, Error>;

enum class JsonType : std::int8_t { Null, Boolean, Number, String, Array, Object, COUNT_ };

struct SerializeFlag {
	enum : std::uint8_t {
		None = 0,
		SortKeys = 1 << 0,
		TrailingNewline = 1 << 1,
		NoSpaces = 1 << 2,
	};
};
using SerializeFlags = decltype(std::to_underlying(SerializeFlag::None));
inline constexpr auto serialize_flags_v = SerializeFlag::SortKeys | SerializeFlag::TrailingNewline;

struct SerializeOptions {
	std::string_view indent{"  "};
	std::string_view newline{"\n"};
	SerializeFlags flags{serialize_flags_v};
};

namespace detail {
struct Value;
class Parser;
} // namespace detail

class Json {
  public:
	using Type = JsonType;

	Json() = default;
	~Json() = default;

	Json(Json&&) = default;
	auto operator=(Json&&) -> Json& = default;

	Json(Json const& other);
	auto operator=(Json const& other) -> Json&;

	explicit(false) Json(std::nullptr_t) noexcept {}

	template <SettableT Type>
	explicit(false) Json(Type const& value) {
		set(value);
	}

	[[nodiscard]] static auto parse(std::string_view text) -> Result;
	[[nodiscard]] static auto from_file(std::string_view path) -> Result;

	[[nodiscard]] static auto empty_array() -> Json const&;
	[[nodiscard]] static auto empty_object() -> Json const&;

	[[nodiscard]] auto get_type() const -> Type;

	[[nodiscard]] auto is_null() const -> bool { return get_type() == Type::Null; }
	[[nodiscard]] auto is_boolean() const -> bool { return get_type() == Type::Boolean; }
	[[nodiscard]] auto is_number() const -> bool { return get_type() == Type::Number; }
	[[nodiscard]] auto is_string() const -> bool { return get_type() == Type::String; }
	[[nodiscard]] auto is_array() const -> bool { return get_type() == Type::Array; }
	[[nodiscard]] auto is_object() const -> bool { return get_type() == Type::Object; }

	[[nodiscard]] auto as_bool(bool fallback = {}) const -> bool;
	[[nodiscard]] auto as_double(double fallback = {}) const -> double;
	[[nodiscard]] auto as_u64(std::uint64_t fallback = {}) const -> std::uint64_t;
	[[nodiscard]] auto as_i64(std::int64_t fallback = {}) const -> std::int64_t;

	[[nodiscard]] auto as_string_view(std::string_view fallback = {}) const -> std::string_view;
	[[nodiscard]] auto as_string(std::string_view const fallback = {}) const -> std::string { return std::string{as_string_view(fallback)}; }

	template <NumericT Type>
	[[nodiscard]] auto as_number(Type const fallback = {}) const -> Type {
		if constexpr (std::signed_integral<Type>) {
			return static_cast<Type>(as_i64(static_cast<std::int64_t>(fallback)));
		} else if constexpr (std::unsigned_integral<Type>) {
			return static_cast<Type>(as_u64(static_cast<std::uint64_t>(fallback)));
		} else {
			return static_cast<Type>(as_double(static_cast<double>(fallback)));
		}
	}

	template <GettableT Type>
	[[nodiscard]] auto as(Type const& fallback = {}) const -> Type {
		if constexpr (std::same_as<Type, bool>) {
			return as_bool(fallback);
		} else if constexpr (NumericT<Type>) {
			return as_number(fallback);
		} else if constexpr (std::same_as<Type, std::string_view>) {
			return as_string_view(fallback);
		} else {
			return as_string(fallback);
		}
	}

	[[nodiscard]] auto as_array() const -> std::span<dj::Json const>;
	[[nodiscard]] auto as_object() const -> StringTable<dj::Json> const&;

	void set_null();
	void set_boolean(bool value);
	void set_string(std::string_view value);
	void set_number(std::int64_t value);
	void set_number(std::uint64_t value);
	void set_number(double value);
	void set_value(Json value);

	template <NumericT Type>
	void set_number(Type const value) {
		if constexpr (std::signed_integral<Type>) {
			set_number(static_cast<std::int64_t>(value));
		} else if constexpr (std::unsigned_integral<Type>) {
			set_number(static_cast<std::uint64_t>(value));
		} else {
			set_number(static_cast<double>(value));
		}
	}

	template <SettableT Type>
	void set(Type const& value) {
		if constexpr (std::same_as<Type, std::nullptr_t>) {
			set_null();
		} else if constexpr (std::same_as<Type, bool>) {
			set_boolean(value);
		} else if constexpr (NumericT<Type>) {
			set_number(value);
		} else {
			set_string(value);
		}
	}

	void set_array();
	void set_object();

	auto push_back(Json value = {}) -> Json&;
	auto insert_or_assign(std::string key, Json value) -> Json&;

	[[nodiscard]] auto operator[](std::string_view key) const -> Json const&;
	[[nodiscard]] auto operator[](std::string_view key) -> Json&;

	[[nodiscard]] auto operator[](std::size_t index) const -> Json const&;
	[[nodiscard]] auto operator[](std::size_t index) -> Json&;

	[[nodiscard]] auto serialize(SerializeOptions const& options = {}) const -> std::string;
	[[nodiscard]] auto to_file(std::string_view path, SerializeOptions const& options = {}) const -> bool;

	friend void swap(Json& a, Json& b) noexcept { std::swap(a.m_value, b.m_value); }

	explicit operator bool() const { return m_value != nullptr; }

  private:
	struct Serializer;

	struct Deleter {
		void operator()(detail::Value* ptr) const noexcept;
	};

	void ensure_impl();

	std::unique_ptr<detail::Value, Deleter> m_value;

	friend class detail::Parser;
};

[[nodiscard]] inline auto to_string(Json const& json, SerializeOptions const& options = {}) { return json.serialize(options); }

[[nodiscard]] auto make_escaped(std::string_view text) -> std::string;

template <GettableT Type>
void from_json(Json const& json, Type& value, Type const fallback = {}) {
	value = json.as<Type>(fallback);
}

template <SettableT Type>
void to_json(Json& json, Type const& value) {
	json.set(value);
}
} // namespace dj

template <>
struct std::formatter<dj::Json> {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}

	static auto format(dj::Json const& json, std::format_context& fc) -> std::format_context::iterator;
};
