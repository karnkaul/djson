#pragma once
#include <optional>
#include <ostream>
#include <dumb_json/serial_opts.hpp>
#include <dumb_json/types.hpp>

namespace dj {
///
/// \brief Type of a node
///
enum class node_type { unknown, scalar, object, array };

class node_t {
  public:
	///
	/// \brief Attempt to parse first root node in text
	///
	static std::optional<node_t> make(std::string_view text);
	///
	/// \brief Create a node_ptr
	///
	static node_ptr make_ptr(node_t* to_move = nullptr);
	///
	/// \brief Obtain all root nodes in passed string
	///
	static std::vector<node_t> read(std::string_view text);

	///
	/// \brief Retrieve value(s) as T
	///
	template <typename T>
	T as() const;
	///
	/// \brief Check if object contains a value associated with key
	///
	bool contains(std::string const& key) const;
	///
	/// \brief Obtain node associated with key
	/// Warning: will overwrite existing nodes / create new ones as necessary
	///
	node_t& operator[](std::string const& key);
	///
	/// \brief Obtain node associated with key
	/// Warning: will overwrite existing nodes / create new ones as necessary
	///
	node_t& sub(std::string const& key);
	///
	/// \brief Obtain node associated with key
	/// Warning: will throw on failure
	///
	node_t& get(std::string const& key) const;
	///
	/// \brief Obtain const node associated with key if present, default node otherwise
	///
	node_t const& safe_get(std::string const& key) const;
	///
	/// \brief Obtain node associated with key, if present
	///
	node_t* find(std::string const& key) const;
	///
	/// \brief Overwrite existing value(s)
	///
	void set(value_t&& value);
	///
	/// \brief Overwrite existing value(s)
	///
	void set(array_fields_t<value_t>&& values);
	///
	/// \brief Overwrite existing value(s)
	///
	void set(map_fields_t<value_t>&& values);
	///
	/// \brief Set as array and add value
	///
	node_t& add(value_t&& value);
	///
	/// \brief Set as object and set key to value
	///
	node_t& add(std::string_view key, value_t&& value);
	///
	/// \brief Set as array and add node
	///
	void add(node_t&& node);
	///
	/// \brief Set as object and set key to node
	///
	void add(std::string_view key, node_t&& node);

	///
	/// \brief Obtain the type of this node
	///
	node_type type() const noexcept;
	bool is_scalar() const noexcept;
	bool is_object() const noexcept;
	bool is_array() const noexcept;
	///
	/// \brief Serialise data
	///
	std::ostream& serialise(std::ostream& out, serial_opts_t const& opts = {}, std::uint8_t indent = 0) const;
	///
	/// \brief Serialise to a string
	///
	std::string to_string(serial_opts_t const& opts = {}) const;
	///
	/// \brief Serialise to a stream
	///
	friend std::ostream& operator<<(std::ostream& out, node_t const& node) { return node.serialise(out); }

	value_t m_value;
};

///
/// \brief Specialise this functor to parse scalars into custom types
/// built-in specialisations: std::string, numeric types, bool
///
template <typename T>
struct converter_t {
	T operator()(std::string_view scalar) const noexcept;
};

template <>
struct converter_t<std::string> {
	std::string operator()(std::string_view scalar) const noexcept;
};
template <>
struct converter_t<std::int64_t> {
	std::int64_t operator()(std::string_view scalar) const noexcept;
};
template <>
struct converter_t<std::uint64_t> {
	std::uint64_t operator()(std::string_view scalar) const noexcept;
};
template <>
struct converter_t<double> {
	double operator()(std::string_view scalar) const noexcept;
};
template <>
struct converter_t<bool> {
	bool operator()(std::string_view scalar) const noexcept;
};
} // namespace dj

// impl

namespace dj {
namespace detail {
template <typename T>
struct convert_wrap_t;

template <typename T>
struct convert_wrap_t<array_fields_t<T>> {
	array_fields_t<T> operator()(value_t const& value) const {
		array_fields_t<T> ret;
		if (auto p_arr = std::get_if<field_array_t>(&value)) {
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
				if constexpr (std::is_integral_v<T>) {
					if constexpr (std::is_signed_v<T>) {
						for (auto const& node : *p_arr) {
							if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
								ret.push_back(static_cast<T>(converter_t<std::int64_t>{}(*p_val)));
							}
						}
					} else {
						for (auto const& node : *p_arr) {
							if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
								ret.push_back(static_cast<T>(converter_t<std::uint64_t>{}(*p_val)));
							}
						}
					}
				} else {
					for (auto const& node : *p_arr) {
						if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
							ret.push_back(static_cast<T>(converter_t<double>{}(*p_val)));
						}
					}
				}
			} else if constexpr (std::is_same_v<T, node_ptr>) {
				for (auto const& node : *p_arr) {
					ret.push_back(node);
				}
			} else {
				for (auto const& node : *p_arr) {
					if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
						ret.push_back(converter_t<T>{}(*p_val));
					}
				}
			}
		}
		return ret;
	}
};

template <typename T>
struct convert_wrap_t<map_fields_t<T>> {
	map_fields_t<T> operator()(value_t const& value) const {
		map_fields_t<T> ret;
		if (auto p_map = std::get_if<field_map_t>(&value)) {
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
				if constexpr (std::is_integral_v<T>) {
					if constexpr (std::is_signed_v<T>) {
						for (auto const& [id, node] : *p_map) {
							if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
								ret.emplace(id, static_cast<T>(converter_t<std::int64_t>{}(*p_val)));
							}
						}
					} else {
						for (auto const& [id, node] : *p_map) {
							if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
								ret.emplace(id, static_cast<T>(converter_t<std::uint64_t>{}(*p_val)));
							}
						}
					}
				} else {
					for (auto const& [id, node] : *p_map) {
						if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
							ret.emplace(id, static_cast<T>(converter_t<double>{}(*p_val)));
						}
					}
				}
			} else if constexpr (std::is_same_v<T, node_ptr>) {
				for (auto const& [id, node] : *p_map) {
					ret.emplace(id, node);
				}
			} else {
				for (auto const& [id, node] : *p_map) {
					if (auto p_val = std::get_if<scalar_t>(&node->m_value)) {
						ret.emplace(id, converter_t<T>{}(*p_val));
					}
				}
			}
		}
		return ret;
	}
};

template <typename T>
struct convert_wrap_t {
	T operator()(value_t const& value) const {
		if (auto p_val = std::get_if<scalar_t>(&value)) {
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
				if constexpr (std::is_integral_v<T>) {
					if constexpr (std::is_signed_v<T>) {
						return static_cast<T>(converter_t<std::int64_t>{}(*p_val));
					} else {
						return static_cast<T>(converter_t<std::uint64_t>{}(*p_val));
					}
				} else {
					return static_cast<T>(converter_t<double>{}(*p_val));
				}
			} else {
				return converter_t<T>{}(*p_val);
			}
		}
		return T{};
	}
};
} // namespace detail

template <typename T>
T node_t::as() const {
	return detail::convert_wrap_t<T>{}(m_value);
}
} // namespace dj
