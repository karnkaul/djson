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

constexpr inline std::array g_data_type_str = {"none", "boolean", "integer", "floating", "string", "object", "array"};
inline std::string_view g_newline = "\n";

using log_error = void (*)(std::string_view);
inline log_error g_log_error = nullptr;

struct base
{
	virtual ~base() = default;

	virtual data_type type() const = 0;

	template <typename T>
	T* cast();

	template <typename T>
	T const* cast() const;
};

struct value_type
{
};
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

	bool read(std::string_view text, std::int8_t max_depth = 8, std::uint64_t* line = nullptr);
	base* add(std::string key, std::string_view value, data_type type, std::int8_t max_depth = 8);

	template <typename T>
	T* add(std::string key, std::string_view value, std::int8_t max_depth = 8);

	template <typename T>
	T* find(std::string const& id) const;

	template <typename T>
	decltype(T::value) value(std::string const& id) const;

	std::stringstream& serialise(std::stringstream& out, bool sort_keys, bool pretty, std::uint8_t indent) const;
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

	bool read(std::string_view text, std::uint64_t* line = nullptr);

	template <typename T>
	std::vector<T*> cast() const;

	template <typename T>
	std::vector<decltype(T::value)*> cast_values() const;

	std::stringstream& serialise(std::stringstream& out, bool sort_keys = false, bool pretty = true, std::uint8_t indent = 0) const;
};
} // namespace dj

// Template implementations
#include <dumb_json/dj_traits.hpp>

namespace dj
{
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
	return dynamic_cast<T*>(add(std::move(key), value, to_data_type<T>(), max_depth));
}

template <typename T>
T* object::find(std::string const& id) const
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	if (auto search = entries.find(id); search != entries.end())
	{
		return dynamic_cast<T*>(search->second.get());
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

template <typename T>
std::vector<T*> array::cast() const
{
	static_assert(is_valid_type<T>, "T must derive from base!");
	std::vector<T*> ret;
	ret.reserve(entries.size());
	for (auto const& entry : entries)
	{
		if (auto t_entry = dynamic_cast<T*>(entry.get()))
		{
			ret.push_back(t_entry);
		}
	}
	return ret;
}

template <typename T>
std::vector<decltype(T::value)*> array::cast_values() const
{
	static_assert(is_value_type<T>, "T must be a value type!");
	std::vector<decltype(T::value)*> ret;
	ret.reserve(entries.size());
	for (auto const& entry : entries)
	{
		if (auto t_entry = dynamic_cast<T*>(entry.get()))
		{
			ret.push_back(&t_entry->value);
		}
	}
	return ret;
}
} // namespace dj
