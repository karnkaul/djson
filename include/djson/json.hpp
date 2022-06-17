#pragma once
#include <djson/detail.hpp>
#include <djson/djson_version.hpp>
#include <djson/from_chars.hpp>
#include <djson/result.hpp>
#include <concepts>

namespace dj {
template <typename T>
concept Number = !std::same_as<T, bool> && (std::integral<T> || std::floating_point<T>);

template <typename T>
concept Literal = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string> || std::same_as<T, std::string_view>;

template <typename T>
concept Element = Literal<T> || std::same_as<T, json>;

class json {
  public:
	json() = default;

	///
	/// \brief Set this instance to Literal t
	///
	template <Literal T>
	explicit json(T t);
	///
	/// \brief Assign this instance as Element t
	///
	template <Element T>
	json& operator=(T t);
	json& operator=(char const* str) { return operator=(std::string(str)); }

	///
	/// \brief Read a JSON string
	///
	parse_result read(std::string_view text);
	///
	/// \brief Open file at path and read it as a JSON string
	///
	io_result open(char const* path);

	value_type type() const { return m_value.type; }
	bool is_null() const { return type() == value_type::null; }
	bool is_boolean() const { return type() == value_type::boolean; }
	bool is_number() const { return type() == value_type::number; }
	bool is_string() const { return type() == value_type::string; }
	bool is_array() const { return type() == value_type::array; }
	bool is_object() const { return type() == value_type::object; }

	///
	/// \brief Check if value_type is object and it contains key
	///
	bool contains(std::string const& key) const;
	///
	/// \brief Obtain reference to instance associated with key
	///
	/// Sets instance as object type if not so
	/// Inserts a null instance for key if not present
	///
	json& value(std::string const& key);
	///
	/// \brief Obtain const reference to instance associated with key
	///
	/// Returns null instance if not value_type is not object or if key not present
	///
	json const& value(std::string const& key) const;
	///
	/// \brief Returns size of array/object; only meaningful for corresponding value_types
	///
	std::size_t container_size() const;

	///
	/// \brief Obtain value as a boolean (or fallback if value_type mismatch)
	///
	bool as_boolean(bool fallback = {}) const;
	///
	/// \brief Obtain value as a number (or fallback if value_type mismatch)
	///
	template <Number T>
	T as_number(T fallback = {}) const;
	///
	/// \brief Obtain value as a string (or fallback if value_type mismatch)
	///
	std::string as_string(std::string const& fallback = {}) const;
	///
	/// \brief Obtain value as a string_view (or fallback if value_type mismatch)
	///
	std::string_view as_string_view(std::string_view fallback = {}) const;
	///
	/// \brief View value as an array (a span of json&)
	///
	array_view as_array() const { return *this; }
	///
	/// \brief View value as an object (a span of pair<string_view, json&>)
	///
	object_view as_object() const { return *this; }
	///
	/// \brief Obtain copy of value as a Literal
	///
	template <Literal T>
	T as(T const& fallback = {}) const;

	///
	/// \brief Obtain reference to instance associated with key
	///
	/// Sets instance as object type if not so
	/// Inserts a null instance for key if not present
	///
	json& operator[](std::string const& key) { return value(key); }
	///
	/// \brief Obtain const reference to instance associated with key
	///
	/// Returns null instance if not value_type is not object or if key not present
	///
	json const& operator[](std::string const& key) const { return value(key); }

	///
	/// \brief Serialize value to a string (using serializer)
	///
	std::string serialize(bool pretty_print = true) const;
	///
	/// \brief Write value to file path (using serializer)
	///
	bool write(char const* path, bool pretty_print = true) const;

	///
	/// \brief Set value as null
	///
	json& set_null();
	///
	/// \brief Set value as Element t
	///
	template <Element T>
	json& set(T t);
	///
	/// \brief Push Element t into value as array
	///
	template <Element T>
	json& push_back(T t);
	///
	/// \brief Insert [key, t] into value as object
	///
	template <Element T>
	json& insert(std::string key, T t);

	operator std::string() const { return as_string(); }
	operator std::string_view() const { return as_string_view(); }

  private:
	json(detail::value_t&& value) noexcept : m_value(std::move(value)) {}

	template <typename T>
	static std::unique_ptr<json> make_ujson(T t);

	detail::value_t m_value{};

	template <typename T>
	friend struct detail::facade;
};

///
/// \brief Serializer for a json instance
///
/// Pass an instance to operator<<(std::ostream&, serializer) to write to it
/// Call operator() to write to file path
///
struct serializer {
	json const& doc;
	bool pretty_print{true};

	bool operator()(char const* path) const;
};

///
/// \brief Remove all whitespace
///
std::string minify(std::string_view text);

std::ostream& operator<<(std::ostream& out, serializer const& s);
std::ostream& operator<<(std::ostream& out, parse_result const& result);
std::ostream& operator<<(std::ostream& out, io_result const& result);

// impl

template <Number T>
struct detail::facade<T> {
	T operator()(json const& js, T fallback = {}) const {
		if (!js.is_number()) { return fallback; }
		auto const& str = std::get<std::string>(js.m_value.value);
		T out;
		auto [ptr, ec] = detail::from_chars(str.data(), str.data() + str.size(), out);
		if (ec != std::errc()) { return fallback; }
		return out;
	}
};

template <Literal T>
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
