#pragma once
#include <djson/detail.hpp>
#include <djson/error.hpp>
#include <concepts>

namespace dj {
template <typename Error>
struct result;

template <typename T>
concept Number = !std::same_as<T, bool> && (std::integral<T> || std::floating_point<T>);

template <typename T>
concept Literal = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string> || std::same_as<T, std::string_view>;

template <typename T>
concept Element = Literal<T> || std::same_as<T, json>;

class json {
  public:
	json() = default;

	template <Element T>
	explicit json(T t);
	template <Element T>
	json& operator=(T t);

	result<parse_error> read(std::string_view text);
	result<io_error> open(char const* path);

	value_type type() const { return m_value.type; }
	bool is_null() const { return type() == value_type::null; }
	bool is_boolean() const { return type() == value_type::boolean; }
	bool is_number() const { return type() == value_type::number; }
	bool is_string() const { return type() == value_type::string; }
	bool is_array() const { return type() == value_type::array; }
	bool is_object() const { return type() == value_type::object; }

	bool contains(std::string const& key) const;
	json& value(std::string const& key);
	json const& value(std::string const& key) const;
	///
	/// \brief Returns size of array/object; only meaningful for corresponding value_types
	///
	std::size_t container_size() const;

	bool as_boolean(bool fallback = {}) const;
	template <Number T>
	T as_number(T fallback = {}) const;
	std::string as_string(std::string const& fallback = {}) const;
	std::string_view as_string_view(std::string_view fallback = {}) const;
	array_view as_array() const { return *this; }
	object_view as_object() const { return *this; }
	template <Literal T>
	T as(T const& fallback = {}) const;

	json& operator[](std::string const& key) { return value(key); }
	json const& operator[](std::string const& key) const { return value(key); }

	operator std::string() const { return as_string(); }
	operator std::string_view() const { return as_string_view(); }

	std::string serialize(bool pretty_print = true) const;
	bool write(char const* path, bool pretty_print = true) const;

	json& set_null();
	template <Element T>
	json& set(T t);
	template <Element T>
	json& push_back(T t);
	template <Element T>
	json& insert(std::string key, T t);

  private:
	json(detail::value_t&& value) noexcept : m_value(std::move(value)) {}

	template <typename T>
	static std::unique_ptr<json> make_ujson(T t);

	detail::value_t m_value{};

	template <typename T>
	friend struct detail::facade;
};

template <typename Error>
struct result {
	Error err{};

	explicit operator bool() const { return !err; }
};

struct serializer {
	json const& doc;
	bool pretty_print{true};

	bool operator()(char const* path) const;
};

std::string minify(std::string_view text);
std::ostream& operator<<(std::ostream& out, serializer const& s);

// impl

template <Number T>
struct detail::facade<T> {
	T operator()(json const& js, T fallback = {}) const {
		if (!js.is_number()) { return fallback; }
		auto const& str = std::get<std::string>(js.m_value.value);
		T out;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
		if (ec != std::errc()) { return fallback; }
		return out;
	}
};

template <Element T>
json::json(T t) {
	set(std::move(t));
}

template <Element T>
json& json::operator=(T t) {
	set(std::move(t));
	return *this;
}

template <Number T>
T json::as_number(T fallback) const {
	return detail::facade<T>{}(*this, fallback);
}

template <Literal T>
T json::as(T const& fallback) const {
	return detail::facade<T>{}(*this, fallback);
}

inline json& json::set_null() {
	m_value = detail::null_v;
	return *this;
}

template <Element T>
json& json::set(T t) {
	if constexpr (std::same_as<T, json>) {
		m_value = std::move(t.m_value);
	} else {
		m_value = detail::to_value<T>{}(std::move(t));
	}
	return *this;
}

template <Element T>
json& json::push_back(T t) {
	if (!is_array()) { m_value = {detail::arr_t{}, value_type::array}; }
	auto& arr = std::get<detail::arr_t>(m_value.value);
	arr.nodes.push_back(make_ujson(std::move(t)));
	return *arr.nodes.back();
}

template <Element T>
json& json::insert(std::string key, T t) {
	if (!is_object()) { m_value = {detail::obj_t{}, value_type::object}; }
	auto& obj = std::get<detail::obj_t>(m_value.value);
	auto [it, _] = obj.nodes.insert_or_assign(key, make_ujson(std::move(t)));
	return *it->second;
}

template <typename T>
std::unique_ptr<json> json::make_ujson(T t) {
	auto uj = std::make_unique<json>();
	uj->set(std::move(t));
	return uj;
}
} // namespace dj
