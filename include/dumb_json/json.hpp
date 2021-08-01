#pragma once
#include <exception>
#include <optional>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <variant>
#include <dumb_json/json_type.hpp>
#include <dumb_json/serial_opts.hpp>

namespace dj {
///
/// \brief Exception type thrown on operator[] when key/index doesn't exist
///
struct not_found_exception : std::exception {};
///
/// \brief Type alias for array type (use with as<T>())
///
using vec_t = query_t<array_t>;
///
/// \brief Type alias for object type (use with as<T>())
///
using map_t = query_t<object_t>;

///
/// \brief Models a JSON value, provides primary API
///
class json {
  public:
	enum class error_type { syntax, parse, io };
	static constexpr std::string_view error_type_names[] = {"syntax", "parse", "io"};

	///
	/// \brief Type alias for storage (internal)
	///
	using storage_t = std::variant<null_t, boolean_t, number_t, string_t, array_t, object_t>;

	struct error_t;
	struct result_t;

	///
	/// \brief Default construct
	///
	json() = default;
	///
	/// \brief Construct using an existing value
	///
	explicit json(storage_t value) noexcept : m_value(std::move(value)) {}
	///
	/// \brief Obtain storage value
	///
	storage_t const& value() const noexcept { return m_value; }

	///
	/// \brief Obtain currently held type
	///
	json_type type() const noexcept;
	bool is_null() const noexcept { return type() == json_type::null; }
	bool is_boolean() const noexcept { return type() == json_type::boolean; }
	bool is_number() const noexcept { return type() == json_type::number; }
	bool is_string() const noexcept { return type() == json_type::string; }
	bool is_array() const noexcept { return type() == json_type::array; }
	bool is_object() const noexcept { return type() == json_type::object; }

	///
	/// \brief Parse a JSON string into this object / value
	///
	result_t read(std::string_view text);
	///
	/// \brief Load a JSON file into this object / value
	///
	result_t load(std::string const& path);
	///
	/// \brief Output value into stream
	///
	std::ostream& serialize(std::ostream& out, serial_opts_t const& opts = s_serial_opts) const;
	///
	/// \brief Obtain value as a std::string
	///
	std::string to_string(serial_opts_t const& opts = s_serial_opts) const;
	///
	/// \brief Output value to file at path
	///
	bool save(std::string const& path) const;

	///
	/// \brief Obtain value as C++ type
	///
	template <typename T>
	T as() const;
	///
	/// \brief Check if value is of object type and containing key
	///
	bool contains(std::string const& key) const noexcept;
	///
	/// \brief Check if value is of array type and containing index
	///
	bool contains(std::size_t index) const noexcept;
	///
	/// \brief Obtain value of object type corresponding to key, if present
	///
	json* find(std::string const& key) const noexcept;
	///
	/// \brief Obtain value of array type corresponding to index, if present
	///
	json* find(std::size_t index) const noexcept;
	///
	/// \brief Obtain value as T of object type corresponding to key, if present
	///
	template <typename T>
	std::optional<T> find_as(std::string const& key) const;
	///
	/// \brief Obtain value as T of object type corresponding to key, else fallback
	///
	template <typename T>
	T get_as(std::string const& key, T const& fallback = {}) const;
	///
	/// \brief Obtain value of object type corresponding to key
	/// Warning: throws not_found_exception if not present
	///
	json& operator[](std::string const& key) const noexcept(false);
	///
	/// \brief Obtain value of array type corresponding to index
	/// Warning: throws not_found_exception if not present
	///
	json& operator[](std::size_t index) const noexcept(false);

	///
	/// \brief Set value to null / boolean / number / string type
	///
	template <typename T>
	json& set(T t);
	///
	/// \brief Set value to null / boolean / number / string type
	///
	json& set(json_type type, std::string value);
	///
	/// \brief Set value
	///
	json& set(json value);
	///
	/// \brief Set as array type and push value
	///
	json& push_back(json value);
	///
	/// \brief Set as object type and insert key / value
	///
	json& insert(std::string const& key, json value);
	///
	/// \brief Set as null type
	///
	json& clear() { return set(json_type::null, {}); }

	///
	/// \brief Default serialisation options
	///
	inline static serial_opts_t s_serial_opts;

  private:
	storage_t m_value;

	template <typename T>
	friend struct detail::getter_t;
};

///
/// \brief Error type and message
///
struct json::error_t {
	std::string message;
	error_type type;
};
///
/// \brief Parse result
///
struct json::result_t {
	std::vector<error_t> errors;
	bool failure = true;

	explicit operator bool() const noexcept { return !failure; }
	std::string to_string() const;
};

///
/// \brief Output json value to stream
///
std::ostream& operator<<(std::ostream& out, json const& json);
///
/// \brief Output json result to stream
///
std::ostream& operator<<(std::ostream& out, json::result_t const& result);

// impl

namespace detail {
template <typename T>
T const* to_t(json::storage_t const& value) noexcept {
	return std::get_if<T>(&value);
}

template <typename T>
struct getter_t {
	std::int64_t to_int(std::string const& text) const { return std::stoll(text); }
	std::uint64_t to_uint(std::string const& text) const { return std::stoull(text); }
	double to_double(std::string const& text) const { return std::stod(text); }

	T operator()(json::storage_t const& value) const {
		if constexpr (std::is_same_v<T, bool>) {
			auto val = to_t<boolean_t>(value);
			return val ? val->value : false;
		} else if constexpr (std::is_arithmetic_v<T>) {
			T ret{};
			if (auto val = to_t<number_t>(value)) {
				std::stringstream str(val->value);
				str >> ret;
			}
			return ret;
		} else if constexpr (std::is_same_v<T, std::string>) {
			auto val = to_t<string_t>(value);
			return val ? val->value : std::string();
		} else {
			return T{};
		}
	}
};

template <typename T>
struct getter_t<std::vector<T>> {
	std::vector<T> operator()(json::storage_t const& value) const {
		std::vector<T> ret;
		if (auto val = to_t<array_t>(value)) {
			for (auto const& node : val->value) {
				if constexpr (std::is_same_v<T, ptr<json>>) {
					ret.push_back(node);
				} else if constexpr (std::is_same_v<std::remove_const_t<T>, json*>) {
					ret.push_back(node.get());
				} else {
					ret.push_back(getter_t<T>{}(node->m_value));
				}
			}
		}
		return ret;
	}
};

template <typename T>
struct setter {
	void operator()(json::storage_t& out, [[maybe_unused]] T t) const {
		if constexpr (std::is_same_v<T, std::nullptr_t>) {
			out = null_t{};
		} else if constexpr (std::is_same_v<T, bool>) {
			out = boolean_t{t};
		} else if constexpr (std::is_same_v<T, std::string>) {
			out = string_t{std::move(t)};
		} else {
			std::stringstream str;
			str << t;
			if constexpr (std::is_arithmetic_v<T>) {
				out = number_t{str.str()};
			} else {
				out = string_t{str.str()};
			}
		}
	}
};
} // namespace detail

template <typename T>
T json::as() const {
	if constexpr (std::is_same_v<T, map_t> || std::is_same_v<T, vec_t>) {
		using v_t = std::conditional_t<std::is_same_v<T, map_t>, object_t, array_t>;
		T ret;
		if (auto t = detail::to_t<v_t>(m_value)) { ret = t->value; }
		return ret;
	} else {
		return detail::getter_t<T>{}(m_value);
	}
}

template <typename T>
std::optional<T> json::find_as(std::string const& key) const {
	if (auto ret = find(key)) { return ret->as<T>(); }
	return std::nullopt;
}

template <typename T>
T json::get_as(std::string const& key, T const& fallback) const {
	if (auto ret = find(key)) { return ret->as<T>(); }
	return fallback;
}

template <typename T>
json& json::set(T t) {
	detail::setter<T>{}(m_value, std::move(t));
	return *this;
}
} // namespace dj
