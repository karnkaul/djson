#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace dj {
///
/// \brief Value type of json
///
enum class json_type { null, boolean, number, string, array, object };

///
/// \brief Type alias for std::shared_ptr<T>
///
template <typename T>
using ptr = std::shared_ptr<T>;

///
/// \brief Null type
///
struct null_t {
	using storage_t = void;
	static constexpr json_type type = json_type::null;
};
///
/// \brief Boolean type
///
struct boolean_t {
	using storage_t = bool;
	static constexpr json_type type = json_type::boolean;

	storage_t value = {};
};
///
/// \brief Number type (stored as std::string)
///
struct number_t {
	using storage_t = std::string;
	static constexpr json_type type = json_type::number;

	storage_t value;
};
///
/// \brief String type
///
struct string_t {
	using storage_t = std::string;
	static constexpr json_type type = json_type::string;

	storage_t value;
};
///
/// \brief Value type
///
class json_t; // defined in json.hpp
///
/// \brief Array type
///
struct array_t {
	using storage_t = std::vector<ptr<json_t>>;
	static constexpr json_type type = json_type::array;

	storage_t value;
};
///
/// \brief Object type
///
struct object_t {
	using storage_t = std::unordered_map<std::string, ptr<json_t>>;
	static constexpr json_type type = json_type::object;

	storage_t value;
};

///
/// \brief Type alias for storage of a json value type
///
template <typename T>
using query_t = typename T::storage_t;

namespace detail {
template <typename T>
struct getter_t;
} // namespace detail
} // namespace dj
