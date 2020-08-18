#pragma once
#include <type_traits>
#include <dumb_json/dumb_json.hpp>

namespace dj
{
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
} // namespace dj
