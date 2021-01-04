#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dj {
///
/// \brief Models a JSON value
///
class node_t;
using node_ptr = std::shared_ptr<node_t>;

///
/// \brief Storage for node_type::object
///
using field_map_t = std::unordered_map<std::string, node_ptr>;
///
/// \brief Storage for node_type::array
///
using field_array_t = std::vector<node_ptr>;
///
/// \brief Storage for node_type::scalar
///
using scalar_t = std::string;
///
/// \brief Variant for node's data
///
using value_t = std::variant<scalar_t, field_map_t, field_array_t>;

///
/// \brief Alias for reading values as objects
///
template <typename T>
using map_fields_t = std::unordered_map<std::string_view, T>;
using map_nodes_t = map_fields_t<node_ptr>;
///
/// \brief Alias for reading values as arrays
///
template <typename T>
using array_fields_t = std::vector<T>;
using array_nodes_t = array_fields_t<node_ptr>;
} // namespace dj
