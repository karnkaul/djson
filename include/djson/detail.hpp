#pragma once
#include <charconv>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dj {
class json;
using ujson = std::unique_ptr<json>;
enum class value_type { null, boolean, number, string, array, object };

template <typename T>
class ref {
  public:
	constexpr ref(T& t) : m_t(&t) {}

	constexpr T& get() const { return *m_t; }
	constexpr operator T&() const { return *m_t; }
	constexpr operator ref<T const>() const { return {*m_t}; }

  private:
	T* m_t;
};
} // namespace dj

namespace dj::detail {
struct fifo_map_t {
	std::unordered_map<std::string, std::size_t> indices{};
	std::vector<std::pair<std::string, ujson>> entries{};

	using iterator = decltype(entries)::iterator;
	using const_iterator = decltype(entries)::const_iterator;

	std::pair<iterator, bool> insert_or_assign(std::string const& key, ujson value);
	const_iterator find(std::string const& key) const;
	ujson& operator[](std::string const& key);

	bool empty() const { return entries.empty(); }
	std::size_t size() const { return entries.size(); }
	std::pair<std::string, ujson> const& operator[](std::size_t index) const { return entries[index]; }

	auto begin() { return entries.begin(); }
	auto end() { return entries.end(); }
	auto begin() const { return entries.begin(); }
	auto end() const { return entries.end(); }
};

using lit_t = std::string;
struct arr_t;
struct obj_t;
using val_t = std::variant<arr_t, obj_t, lit_t>;

struct arr_t {
	std::vector<ujson> nodes{};
};

struct obj_t {
	fifo_map_t nodes{};
};

val_t clone(val_t const& rhs);

struct value_t {
	val_t value{};
	value_type type{value_type::null};

	value_t() = default;
	value_t(val_t value, value_type type) noexcept;
	value_t(value_t&&) = default;
	value_t& operator=(value_t&&) = default;
	value_t(value_t const& rhs);
	value_t& operator=(value_t const& rhs);

	operator val_t&() { return value; }
	operator val_t const&() const { return value; }
};

inline auto const null_v = value_t{{}, value_type::null};

template <typename T>
struct facade;

template <>
struct facade<std::string_view> {
	std::string_view operator()(value_t const& value, std::string_view fallback = {}) const {
		if (auto str = std::get_if<std::string>(&value.value)) { return *str; }
		return fallback;
	}
};

template <>
struct facade<std::string> {
	std::string operator()(value_t const& value, std::string_view fallback = {}) const { return std::string(facade<std::string_view>{}(value, fallback)); }
};

template <>
struct facade<bool> {
	bool operator()(value_t const& value, bool fallback = {}) const { return facade<std::string_view>{}(value) == "true" ? true : fallback; }
};

template <typename T>
requires(std::integral<T> || std::floating_point<T>) struct facade<T> {
	T operator()(value_t const& value, T fallback = {}) const {
		auto str = facade<std::string_view>{}(value);
		if (str.empty()) { return fallback; }
		T out;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
		if (ec != std::errc()) { return fallback; }
		return out;
	}
};

template <typename T>
struct setter_t;

template <>
struct setter_t<std::string> {
	value_t operator()(std::string in) const { return {std::move(in), value_type::string}; }
};

template <>
struct setter_t<std::string_view> {
	value_t operator()(std::string_view in) const { return {std::string(in), value_type::string}; }
};

template <>
struct setter_t<bool> {
	value_t operator()(bool const in) const {
		auto ret = value_t{in ? "true" : "false", value_type::boolean};
		return ret;
	}
};

template <typename T>
requires(std::integral<T> || std::floating_point<T>) struct setter_t<T> {
	value_t operator()(T const value) const {
		char buf[16]{};
		auto [ptr, ec] = std::to_chars(std::begin(buf), std::end(buf), value);
		if (ec != std::errc()) { return {"null", value_type::null}; }
		return {buf, value_type::number};
	}
};
} // namespace dj::detail
