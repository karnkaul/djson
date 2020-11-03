#pragma once
#include <array>
#include <cinttypes>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace dj {
using namespace std::string_view_literals;

///
/// \brief Enumeration of all supported concrete types
///
enum class data_type { none, boolean, integer, uinteger, floating, string, object, array };

///
/// \brief Serialisation options
///
struct serial_opts final {
	///
	/// \brief Set true to sort all keys (costs overhead)
	///
	bool sort_keys = false;
	///
	/// \brief Set false to collapse all whitespace
	///
	bool pretty = true;
	///
	/// \brief Set non-zero to use spaces instead of tabs
	///
	std::uint8_t space_indent = 0;
	///
	/// \brief String used for newlines
	///
	std::string_view newline = "\n";
};

///
/// \brief enum-indexed array of string representations for data_type
///
constexpr inline std::array g_data_type_str = {"none"sv, "boolean"sv, "integer"sv, "floating"sv, "string"sv, "object"sv, "array"sv};
///
/// \brief Default serialisation options
///
inline serial_opts g_serial_opts;

///
/// \brief Default error logger
///
inline void log_error(std::string_view str) {
	std::cerr << str << "\n";
};

///
/// \brief Customisable error logging callback
///
inline auto g_log_error = &log_error;

///
/// \brief Abstract base class for all concrete types
///
struct base {
	virtual ~base() = default;

	virtual data_type type() const = 0;

	///
	/// \brief Cast to a concrete type
	///
	template <typename T>
	T* cast();
	///
	/// \brief Cast to a concrete type
	///
	template <typename T>
	T const* cast() const;
};

namespace detail {
struct value_type {};
struct container_type {};
} // namespace detail

using base_ptr = std::unique_ptr<base>;
using field_map = std::unordered_map<std::string, base_ptr>;
using field_array = std::vector<base_ptr>;

struct boolean : base, detail::value_type {
	using value_type = bool;

	value_type value = {};

	data_type type() const override {
		return data_type::boolean;
	}
};

struct integer : base, detail::value_type {
	using value_type = std::int64_t;

	value_type value = {};

	data_type type() const override {
		return data_type::integer;
	}
};

struct uinteger : base, detail::value_type {
	using value_type = std::uint64_t;

	value_type value = {};

	data_type type() const override {
		return data_type::uinteger;
	}
};

struct floating : base, detail::value_type {
	using value_type = double;

	value_type value = {};

	data_type type() const override {
		return data_type::floating;
	}
};

struct string : base, detail::value_type {
	using value_type = std::string;

	value_type value;

	data_type type() const override {
		return data_type::string;
	}
};

struct object : base, detail::container_type {
	field_map fields;

	data_type type() const override {
		return data_type::object;
	}

	///
	/// \brief Deserialise a JSON string or snippet
	///
	/// Must be of the form `{...}`
	///
	bool read(std::string_view text, std::int8_t max_depth = 8, std::uint64_t* line = nullptr);
	///
	/// \brief Add a custom field
	///
	base* add(std::string_view key, std::string_view value, data_type type, std::int8_t max_depth = 8);
	///
	/// \brief Add a custom field of type T
	///
	template <typename T>
	T* add(std::string_view key, std::string_view value, std::int8_t max_depth = 8);
	///
	/// \brief Add a custom value_type field of type T
	///
	template <typename T>
	T* add(std::string_view key, typename T::value_type&& value);
	///
	/// \brief Add a custom value_type field of type T
	///
	template <typename T>
	T* add(std::string_view key, typename T::value_type const& value);
	///
	/// \brief Add an existing entry (move the instance)
	///
	base* add(std::string_view key, base&& entry);

	///
	/// \brief Find a field of type T
	/// \param id search key
	///
	template <typename T>
	T* find(std::string const& id) const;
	///
	/// \brief Obtain value of value_type field
	/// \param id search key
	///
	template <typename T>
	typename T::value_type const& value(std::string const& id) const;
	///
	/// \brief Check if a key is present
	///
	bool contains(std::string const& id) const;
	///
	/// \brief Check if a key of type T is present
	///
	template <typename T>
	bool contains(std::string const& id) const;

	///
	/// \brief Serialise fields into JSON string (adds escapes)
	///
	std::ostream& serialise(std::ostream& out, serial_opts const& options = g_serial_opts, std::uint8_t indent = 0) const;
	///
	/// \brief Serialise fields into JSON string (adds escapes)
	///
	std::string serialise(serial_opts const& options = g_serial_opts, std::uint8_t indent = 0) const;
};

struct array : base, detail::container_type {
	field_array fields;
	data_type held_type = data_type::none;

	data_type type() const override {
		return data_type::array;
	}

	///
	/// \brief Deserialise JSON snippet
	///
	/// Must be of the form `[...]`
	///
	bool read(std::string_view text, std::uint64_t* line = nullptr);
	///
	/// \brief Add an entry (must match held type if not empty)
	///
	base* add(std::string_view value, data_type type, std::int8_t max_depth = 8);
	///
	/// \brief Add an entry (must match held type if not empty)
	///
	template <typename T>
	T* add(std::string_view value, std::int8_t max_depth = 8);
	///
	/// \brief Add a value_type entry (must match held type if not empty)
	///
	template <typename T>
	T* add(typename T::value_type&& value);
	///
	/// \brief Add a value_type entry (must match held type if not empty)
	///
	template <typename T>
	T* add(typename T::value_type const& value);
	///
	/// \brief Add an existing entry (move the instance)
	///
	base* add(base&& entry);

	///
	/// \brief Iterate over elements as type T
	/// \param functor passed T& per iteration
	///
	template <typename T, typename F>
	bool for_each(F functor) const;
	///
	/// \brief Iterate over elements as value_type T
	/// \param functor passed T::value& per iteration
	///
	template <typename T, typename F>
	bool for_each_value(F functor) const;

	///
	/// \brief Serialise fields into JSON string (adds escapes)
	///
	std::ostream& serialise(std::ostream& out, serial_opts const& options = g_serial_opts, std::uint8_t indent = 0) const;
};

///
/// \brief Deserialise a string into its appropriate type object
///
base_ptr deserialise(std::string_view text, std::uint8_t max_depth = 8);
///
/// \brief Cast a base instance into a desired type
///
template <typename T>
T* cast(base_ptr& out_base) {
	return out_base ? out_base->template cast<T>() : nullptr;
}

namespace detail {
template <typename T>
constexpr bool always_false = false;

template <data_type T>
struct t_data_type;

template <>
struct t_data_type<data_type::boolean> {
	using type = boolean;
};
template <>
struct t_data_type<data_type::integer> {
	using type = integer;
};
template <>
struct t_data_type<data_type::floating> {
	using type = floating;
};
template <>
struct t_data_type<data_type::string> {
	using type = string;
};
template <>
struct t_data_type<data_type::object> {
	using type = object;
};
template <>
struct t_data_type<data_type::array> {
	using type = array;
};

template <data_type T>
using data_type_t = typename t_data_type<T>::type;

template <typename T>
constexpr bool is_valid_type = std::is_base_of_v<base, std::decay_t<T>>;

template <typename T>
constexpr bool is_value_type = std::is_base_of_v<value_type, std::decay_t<T>>;

template <typename T>
constexpr bool is_container_type = std::is_base_of_v<container_type, std::decay_t<T>>;

template <typename T>
constexpr data_type to_data_type() {
	if constexpr (std::is_same_v<T, boolean>) {
		return data_type::boolean;
	} else if constexpr (std::is_same_v<T, integer>) {
		return data_type::integer;
	} else if constexpr (std::is_same_v<T, uinteger>) {
		return data_type::uinteger;
	} else if constexpr (std::is_same_v<T, floating>) {
		return data_type::floating;
	} else if constexpr (std::is_same_v<T, string>) {
		return data_type::string;
	} else if constexpr (std::is_same_v<T, object>) {
		return data_type::object;
	} else if constexpr (std::is_same_v<T, array>) {
		return data_type::array;
	} else {
		static_assert(always_false<T>, "Invalid type!");
	}
}

constexpr bool is_type_value_type(data_type type) {
	return type > data_type::none && type < data_type::object;
}
} // namespace detail

template <typename T>
T* base::cast() {
	static_assert(detail::is_valid_type<T>, "T must derive from base!");
	return dynamic_cast<T*>(this);
}

template <typename T>
T const* base::cast() const {
	static_assert(detail::is_valid_type<T>, "T must derive from base!");
	return dynamic_cast<T*>(this);
}

template <typename T>
T* object::add(std::string_view key, std::string_view value, std::int8_t max_depth) {
	if (auto entry = add(key, value, detail::to_data_type<T>(), max_depth)) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T>
T* object::add(std::string_view key, typename T::value_type&& value) {
	T t;
	t.value = std::move(value);
	if (auto entry = add(key, std::move(t))) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T>
T* object::add(std::string_view key, typename T::value_type const& value) {
	T t;
	t.value = value;
	if (auto entry = add(key, std::move(t))) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T>
T* object::find(std::string const& id) const {
	static_assert(detail::is_valid_type<T>, "T must derive from base!");
	if (auto search = fields.find(id); search != fields.end()) {
		return search->second->template cast<T>();
	}
	return nullptr;
}

template <typename T>
typename T::value_type const& object::value(std::string const& id) const {
	static_assert(detail::is_value_type<T>, "T must be a value type!");
	static T const default_t{};
	if (auto p_t = find<T>(id)) {
		return p_t->value;
	}
	return default_t.value;
}

template <typename T>
bool object::contains(std::string const& id) const {
	return find<T>(id) != nullptr;
}

template <typename T>
T* array::add(std::string_view value, std::int8_t max_depth) {
	if (auto entry = add(value, detail::to_data_type<T>(), max_depth)) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T>
T* array::add(typename T::value_type&& value) {
	T t;
	t.value = std::move(value);
	if (auto entry = add(std::move(t))) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T>
T* array::add(typename T::value_type const& value) {
	T t;
	t.value = value;
	if (auto entry = add(std::move(t))) {
		return entry->template cast<T>();
	}
	return nullptr;
}

template <typename T, typename F>
bool array::for_each(F functor) const {
	static_assert(detail::is_valid_type<T>, "T must derive from base!");
	if (held_type == detail::to_data_type<T>()) {
		for (auto const& entry : fields) {
			if (auto t_entry = entry->template cast<T>()) {
				functor(*t_entry);
			}
		}
		return true;
	}
	return false;
}

template <typename T, typename F>
bool array::for_each_value(F functor) const {
	static_assert(detail::is_value_type<T>, "T must be a value type!");
	if (held_type == detail::to_data_type<T>()) {
		for (auto const& entry : fields) {
			if (auto t_entry = entry->template cast<T>()) {
				functor(t_entry->value);
			}
		}
		return true;
	}
	return false;
}
} // namespace dj
