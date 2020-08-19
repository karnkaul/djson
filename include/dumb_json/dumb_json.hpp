#pragma once
#include <array>
#include <cinttypes>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace dj
{
///
/// \brief Enumeration of all supported concrete types
///
enum class data_type
{
	none,
	boolean,
	integer,
	floating,
	string,
	object,
	array
};

///
/// \brief enum-indexed array of string representations for data_type
///
constexpr inline std::array g_data_type_str = {"none", "boolean", "integer", "floating", "string", "object", "array"};
///
/// \brief String used for newlines during serialisation
///
inline std::string_view g_newline = "\n";

///
/// \brief Typedef for error logging callback
///
using log_error = void (*)(std::string_view);
///
/// \brief Error logging callback (dumps to `std::cerr` by default)
///
inline log_error g_log_error = nullptr;

///
/// \brief Abstract base class for all concrete types
///
struct base
{
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

///
/// \brief Tag for value types: boolean, integer, floating, string
///
struct value_type
{
};
///
/// \brief Tag for container types: object, array
///
struct container_type
{
};

using field_map = std::unordered_map<std::string, std::unique_ptr<base>>;
using field_array = std::vector<std::unique_ptr<base>>;

struct boolean : base, value_type
{
	bool value = {};

	data_type type() const override
	{
		return data_type::boolean;
	}
};

struct integer : base, value_type
{
	std::int64_t value = {};

	data_type type() const override
	{
		return data_type::integer;
	}
};

struct floating : base, value_type
{
	double value = {};

	data_type type() const override
	{
		return data_type::floating;
	}
};

struct string : base, value_type
{
	std::string value;

	data_type type() const override
	{
		return data_type::string;
	}
};

struct object : base, container_type
{
	field_map entries;

	data_type type() const override
	{
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
	base* add(std::string key, std::string_view value, data_type type, std::int8_t max_depth = 8);
	///
	/// \brief Add a custom field of type T
	///
	template <typename T>
	T* add(std::string key, std::string_view value, std::int8_t max_depth = 8);

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
	decltype(T::value) value(std::string const& id) const;

	///
	/// \brief Serialise fields into JSON string (adds escapes)
	///
	std::stringstream& serialise(std::stringstream& out, bool sort_keys, bool pretty, std::uint8_t indent) const;
	///
	/// \brief Serialise fields into JSON string (adds escapes)
	///
	std::string serialise(bool sort_keys = false, bool pretty = true, std::uint8_t indent = 0) const;
};

struct array : base, container_type
{
	field_array entries;
	data_type held_type = data_type::none;

	data_type type() const override
	{
		return data_type::array;
	}

	///
	/// \brief Deserialise JSON snippet
	///
	/// Must be of the form `[...]`
	///
	bool read(std::string_view text, std::uint64_t* line = nullptr);

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
	std::stringstream& serialise(std::stringstream& out, bool sort_keys = false, bool pretty = true, std::uint8_t indent = 0) const;
};

template <typename T>
constexpr bool always_false = false;

template <data_type T>
struct t_data_type;

template <>
struct t_data_type<data_type::boolean>
{
	using type = boolean;
};
template <>
struct t_data_type<data_type::integer>
{
	using type = integer;
};
template <>
struct t_data_type<data_type::floating>
{
	using type = floating;
};
template <>
struct t_data_type<data_type::string>
{
	using type = string;
};
template <>
struct t_data_type<data_type::object>
{
	using type = object;
};
template <>
struct t_data_type<data_type::array>
{
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
constexpr data_type to_data_type()
{
	if constexpr (std::is_same_v<T, boolean>)
	{
		return data_type::boolean;
	}
	else if constexpr (std::is_same_v<T, integer>)
	{
		return data_type::integer;
	}
	else if constexpr (std::is_same_v<T, floating>)
	{
		return data_type::floating;
	}
	else if constexpr (std::is_same_v<T, string>)
	{
		return data_type::string;
	}
	else if constexpr (std::is_same_v<T, object>)
	{
		return data_type::object;
	}
	else if constexpr (std::is_same_v<T, array>)
	{
		return data_type::array;
	}
	else
	{
		static_assert(always_false<T>, "Invalid type!");
	}
}

constexpr bool is_type_value_type(data_type type)
{
	return type == data_type::boolean || type == data_type::integer || type == data_type::floating || type == data_type::string;
}

template <typename T>
T* base::cast()
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	return dynamic_cast<T*>(this);
}

template <typename T>
T const* base::cast() const
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	return dynamic_cast<T*>(this);
}

template <typename T>
T* object::add(std::string key, std::string_view value, std::int8_t max_depth)
{
	return add(std::move(key), value, to_data_type<T>(), max_depth)->template cast<T>();
}

template <typename T>
T* object::find(std::string const& id) const
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	if (auto search = entries.find(id); search != entries.end())
	{
		return search->second->template cast<T>();
	}
	return nullptr;
}

template <typename T>
decltype(T::value) object::value(std::string const& id) const
{
	static_assert(is_value_type<T>, "T must be a value type!");
	static T const default_t{};
	if (auto p_t = find<T>(id))
	{
		return p_t->value;
	}
	return default_t.value;
}

template <typename T, typename F>
bool array::for_each(F functor) const
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	if (held_type == to_data_type<T>())
	{
		for (auto const& entry : entries)
		{
			if (auto t_entry = entry->template cast<T>())
			{
				functor(*t_entry);
			}
		}
		return true;
	}
	return false;
}

template <typename T, typename F>
bool array::for_each_value(F functor) const
{
	static_assert(is_value_type<T>, "T must be a value type!");
	if (held_type == to_data_type<T>())
	{
		for (auto const& entry : entries)
		{
			if (auto t_entry = entry->template cast<T>())
			{
				functor(t_entry->value);
			}
		}
		return true;
	}
	return false;
}
} // namespace dj
