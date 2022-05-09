#pragma once
#include <charconv>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dj {
class json;
using ujson = std::unique_ptr<json>;
enum class value_type { null, boolean, number, string, array, object };

class array_view;
class object_view;

namespace detail {
struct fifo_map_t {
	std::unordered_map<std::string, std::size_t> indices{};
	std::vector<std::pair<std::string, ujson>> entries{};

	using iterator = decltype(entries)::iterator;
	using const_iterator = decltype(entries)::const_iterator;

	std::pair<iterator, bool> insert_or_assign(std::string const& key, ujson value);
	const_iterator find(std::string const& key) const;
	ujson& operator[](std::string const& key);

	bool empty() const { return entries.empty(); }
	std::size_t size() const { return entries.size(); }
	std::pair<std::string, ujson> const& operator[](std::size_t index) const { return entries[index]; }

	auto begin() { return entries.begin(); }
	auto end() { return entries.end(); }
	auto begin() const { return entries.begin(); }
	auto end() const { return entries.end(); }
};

using lit_t = std::string;
struct arr_t;
struct obj_t;
using val_t = std::variant<arr_t, obj_t, lit_t>;

struct arr_t {
	std::vector<ujson> nodes{};
};

struct obj_t {
	fifo_map_t nodes{};
};

val_t clone(val_t const& rhs);

struct value_t {
	val_t value{};
	value_type type{value_type::null};

	value_t() = default;
	value_t(val_t value, value_type type) noexcept;
	value_t(value_t&&) = default;
	value_t& operator=(value_t&&) = default;
	value_t(value_t const& rhs);
	value_t& operator=(value_t const& rhs);

	operator val_t&() { return value; }
	operator val_t const&() const { return value; }
};

inline auto const null_v = value_t{{}, value_type::null};

template <typename T>
struct facade;

template <>
struct facade<std::string_view> {
	std::string_view operator()(json const& js, std::string_view fallback = {}) const;
};

template <>
struct facade<std::string> {
	std::string operator()(json const& js, std::string const& fallback = {}) const;
};

template <>
struct facade<bool> {
	bool operator()(json const& js, bool fallback = {}) const;
};

template <typename T>
struct setter_t;

template <>
struct setter_t<std::string> {
	value_t operator()(std::string in) const { return {std::move(in), value_type::string}; }
};

template <>
struct setter_t<std::string_view> {
	value_t operator()(std::string_view in) const { return {std::string(in), value_type::string}; }
};

template <>
struct setter_t<bool> {
	value_t operator()(bool const in) const {
		auto ret = value_t{in ? "true" : "false", value_type::boolean};
		return ret;
	}
};

template <typename T>
requires(std::integral<T> || std::floating_point<T>) struct setter_t<T> {
	value_t operator()(T const value) const {
		char buf[16]{};
		auto [ptr, ec] = std::to_chars(std::begin(buf), std::end(buf), value);
		if (ec != std::errc()) { return {"null", value_type::null}; }
		return {buf, value_type::number};
	}
};

template <typename T>
struct crtp_iterator {
	using difference_type = std::ptrdiff_t;

	T& get() { return static_cast<T&>(*this); }
	T const& get() const { return static_cast<T const&>(*this); }

	decltype(auto) operator*() const { return get().value(); }

	T& operator++() {
		get().increment();
		return get();
	}

	T operator++(int) {
		auto ret = get();
		operator++();
		return ret;
	}

	T& operator--() {
		get().decrement();
		return get();
	}

	T operator--(int) {
		auto ret = get();
		operator--();
		return ret;
	}
};

template <typename T, typename It>
struct crtp_view {
	using iterator = It;
	using const_iterator = It;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = reverse_iterator;

	std::size_t size() const {
		if (auto t = get().m_value) { return t->nodes.size(); }
		return 0;
	}

	bool empty() const { return size() == 0; }

	iterator begin() const {
		auto ret = iterator{};
		if (auto t = get().m_value) { ret.m_it = t->nodes.begin(); }
		return ret;
	}

	iterator end() const {
		auto ret = iterator{};
		if (auto t = get().m_value) { ret.m_it = t->nodes.end(); }
		return ret;
	}

	reverse_iterator rbegin() const { return reverse_iterator(end()); }
	reverse_iterator rend() const { return reverse_iterator(begin()); }

	T const& get() const { return static_cast<T const&>(*this); }
};
} // namespace detail

class array_view_iterator : public detail::crtp_iterator<array_view_iterator> {
  public:
	using it_t = decltype(detail::arr_t{}.nodes)::const_iterator;
	using value_type = json;
	using reference = json&;
	using pointer = json*;

	array_view_iterator() = default;

	bool operator==(array_view_iterator const& rhs) const { return m_it == rhs.m_it; }
	pointer operator->() const { return std::addressof(value()); }
	reference value() const { return **m_it; }
	void increment() { ++m_it; }
	void decrement() { --m_it; }

  private:
	it_t m_it{};
	friend struct detail::crtp_view<array_view, array_view_iterator>;
};

class object_view_iterator : public detail::crtp_iterator<object_view_iterator> {
  public:
	using it_t = decltype(detail::obj_t{}.nodes)::const_iterator;
	using value_type = std::pair<std::string_view, json&>;
	using reference = value_type;

	object_view_iterator() = default;

	struct pointer {
		reference self;
		reference* operator->() { return &self; }
	};

	bool operator==(object_view_iterator const& rhs) const { return m_it == rhs.m_it; }
	pointer operator->() const { return {value()}; }
	reference value() const { return {m_it->first, *m_it->second}; }
	void increment() { ++m_it; }
	void decrement() { --m_it; }

  private:
	it_t m_it{};
	friend struct detail::crtp_view<object_view, object_view_iterator>;
};

class array_view : public detail::crtp_view<array_view, array_view_iterator> {
  public:
	array_view() = default;
	array_view(json const& json);

	json const& operator[](std::size_t index) const;

  private:
	detail::arr_t const* m_value{};
	friend struct crtp_view<array_view, array_view_iterator>;
};

class object_view : public detail::crtp_view<object_view, object_view_iterator> {
  public:
	object_view() = default;
	object_view(json const& json);

  private:
	detail::obj_t const* m_value{};
	friend struct crtp_view<object_view, object_view_iterator>;
};
} // namespace dj
