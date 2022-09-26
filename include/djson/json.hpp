#pragma once
#include <djson/build_version.hpp>
#include <djson/parse_error.hpp>
#include <concepts>
#include <memory>
#include <string>

namespace dj {
template <typename Type>
concept Numeric = (std::integral<Type> && !std::same_as<Type, bool>) || std::floating_point<Type>;

template <typename Type>
concept Stringy = std::convertible_to<Type, std::string_view>;

struct Boolean {
	bool value{};

	explicit constexpr operator bool() const { return value; }
	bool operator==(Boolean const&) const = default;
};

inline constexpr auto true_v = Boolean{true};
inline constexpr auto false_v = Boolean{false};

template <typename Type>
concept Primitive = std::same_as<Type, bool> || std::same_as<Type, Boolean> || Numeric<Type> || Stringy<Type>;

class Json;

std::string to_string(Json const& json);
std::ostream& operator<<(std::ostream& out, Json const& json);

///
/// \brief Library provided from_json overload for Primitives
///
template <Primitive T>
void from_json(Json const& json, T& out, T const& fallback = T{});

///
/// \brief Library provided to_json overload for Primitives
///
template <Primitive T>
void to_json(Json& out, T const& t);

class Json {
  public:
	class ObjectProxy;
	class ArrayProxy;

	Json() noexcept;
	Json(Json&&) noexcept;
	Json(Json const&);
	Json& operator=(Json const) noexcept;
	~Json() noexcept;

	///
	/// \brief Probibit native bool entirely
	///
	Json(bool) = delete;

	///
	/// \brief Attempt to parse text into a Json instance
	/// \returns Json value/tree if successfully parsed, null if a parse error was encountered
	///
	static Json parse(std::string_view text, ParseError::Handler* handler = {});
	///
	/// \brief Attempt to parse text in a file at path into a Json instance
	/// \returns Json value/tree if file opened and text successfully parsed, null otherwise
	///
	static Json from_file(char const* path, ParseError::Handler* handler = {});

	explicit(false) Json(std::nullptr_t);
	explicit(false) Json(Boolean boolean);

	template <Numeric T>
	explicit(false) Json(T const t) : Json{AsNumber{}, std::to_string(t)} {}

	template <Stringy T>
	explicit(false) Json(T const& string) : Json{AsString{}, std::string_view{string}} {}

	[[nodiscard]] bool is_null() const;
	[[nodiscard]] bool is_bool() const;
	[[nodiscard]] bool is_number() const;
	[[nodiscard]] bool is_string() const;
	[[nodiscard]] bool is_object() const;
	[[nodiscard]] bool is_array() const;

	[[nodiscard]] Boolean as_bool(Boolean fallback = {}) const;
	[[nodiscard]] std::int64_t as_i64(std::int64_t fallback = {}) const;
	[[nodiscard]] std::uint64_t as_u64(std::uint64_t fallback = {}) const;
	[[nodiscard]] double as_double(double fallback = {}) const;
	[[nodiscard]] std::string_view as_string(std::string_view fallback = {}) const;

	template <Primitive Type>
	Type as(Type fallback = {}) const {
		return get_as<Type>(fallback);
	}

	///
	/// \brief Iterate over mutable elements as an array (empty if not an array)
	///
	ArrayProxy& array_view();
	///
	/// \brief Iterate over immutable elements as an array (empty if not an array)
	///
	ArrayProxy const& array_view() const;
	///
	/// \brief Iterate over mutable key-value pairs as an object (empty if not an object)
	///
	ObjectProxy& object_view();
	///
	/// \brief Iterate over immutable key-value pairs as an object (empty if not an object)
	///
	ObjectProxy const& object_view() const;

	///
	/// \brief Insert value into array (converted to array if not one)
	///
	Json& push_back(Json value);
	///
	/// \brief Access mutable value at index in array (returns null if not an array / out of bounds)
	///
	Json& operator[](std::size_t index);
	///
	/// \brief Access immutable value at index in array (returns null if not an array / out of bounds)
	///
	Json const& operator[](std::size_t index) const;

	///
	/// \brief Check if key exists as an object (returns false if not an object)
	///
	[[nodiscard]] bool contains(std::string_view key) const;
	///
	/// \brief Assign value to key as an object (converted to object if not one)
	///
	Json& assign(std::string_view key, Json value);
	///
	/// \brief Access mutable value at key in object (returns null if not an object / out of bounds)
	///
	Json& operator[](std::string_view key);
	///
	/// \brief Access immutable value at key in object (returns null if not an object / out of bounds)
	///
	Json const& operator[](std::string_view key) const;

	///
	/// \brief Serialize contained value into a string
	///
	std::string serialized(std::size_t indent_spacing = 2) const;
	///
	/// \brief Serialize contained value to out
	///
	std::ostream& serialize_to(std::ostream& out, std::size_t indent_spacing = 2) const;
	///
	/// \brief Serialize contained value to text file at path
	///
	bool to_file(char const* path, std::size_t indent_spacing = 2) const;

	///
	/// \brief Check if value is not null
	///
	explicit operator bool() const { return !is_null(); }

	void swap(Json& rhs) noexcept;

  private:
	struct Construct {};
	struct AsNumber {};
	struct AsString {};
	class IterBase;
	class ArrayIter;
	class ObjectIter;
	struct Impl;
	struct Serializer;

	Json(Construct);
	Json(AsNumber, std::string number);
	Json(AsString, std::string_view string);

	template <typename T>
	T from_number(T fallback) const;

	template <typename T>
	T get_as(T const& fallback) const;

	std::unique_ptr<Impl> m_impl{};
};

class Json::IterBase {
  public:
	IterBase() = default;
	IterBase& operator++();
	bool operator==(IterBase const&) const = default;

  protected:
	IterBase(Impl* impl, std::size_t index) : m_impl{impl}, m_index{index} {}
	Impl* m_impl{};
	std::size_t m_index{};
};

class Json::ArrayIter : public IterBase {
  public:
	using value_type = Json;
	ArrayIter() = default;
	Json& operator*() const;
	ArrayIter& operator++() { return (++static_cast<IterBase&>(*this), *this); }

  private:
	using IterBase::IterBase;
	friend class Json;
};

class Json::ObjectIter : public IterBase {
  public:
	using value_type = std::pair<std::string_view, Json&>;
	ObjectIter() = default;
	value_type operator*() const;
	ObjectIter& operator++() { return (++static_cast<IterBase&>(*this), *this); }

  private:
	using IterBase::IterBase;
	friend class ObjectProxy;
};

class Json::ArrayProxy {
	template <bool Const>
	class Iter;

  public:
	using iterator = Iter<false>;
	using const_iterator = Iter<true>;

	ArrayProxy() = default;
	ArrayProxy& operator=(ArrayProxy&&) = delete;

	bool empty() const;
	std::size_t size() const;

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

  private:
	ArrayProxy(Impl* impl) : m_impl{impl} {}

	ArrayIter array_begin() const;
	ArrayIter array_end() const;

	Impl* m_impl{};
	friend class Json;
};

class Json::ObjectProxy {
	template <bool Const>
	class Iter;

  public:
	using iterator = Iter<false>;
	using const_iterator = Iter<true>;

	ObjectProxy() = default;
	ObjectProxy& operator=(ObjectProxy&&) = delete;

	bool empty() const;
	std::size_t size() const;

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

  private:
	template <typename T>
	struct PtrProxy {
		T value;
		T* operator->() { return &value; }
	};

	ObjectProxy(Impl* impl) : m_impl{impl} {}

	ObjectIter object_begin() const;
	ObjectIter object_end() const;

	Impl* m_impl{};
	friend class Json;
};

template <bool Const>
class Json::ArrayProxy::Iter {
  public:
	using value_type = Json;
	using pointer = std::conditional_t<Const, value_type const*, value_type*>;
	using reference = std::conditional_t<Const, value_type const&, value_type&>;

	Iter() = default;

	reference operator*() const { return *m_it; }
	pointer operator->() const { return &*m_it; }

	Iter& operator++() { return (++m_it, *this); }

	bool operator==(Iter const&) const = default;

  private:
	Iter(ArrayIter it) : m_it{it} {}
	ArrayIter m_it{};
	friend class Json;
};

template <bool Const>
class Json::ObjectProxy::Iter {
  public:
	using value_type = std::conditional_t<Const, std::pair<std::string_view, Json const&>, std::pair<std::string_view, Json&>>;
	using reference = value_type;
	using pointer = PtrProxy<value_type>;

	Iter() = default;

	reference operator*() const { return *m_it; }
	pointer operator->() const { return {*m_it}; }

	Iter& operator++() { return (++m_it, *this); }

	bool operator==(Iter const&) const = default;

  private:
	Iter(ObjectIter it) : m_it{it} {}
	ObjectIter m_it{};
	friend class ObjectProxy;
};

// impl

template <typename T>
T Json::get_as(T const& fallback) const {
	if constexpr (std::same_as<T, bool> || std::same_as<T, Boolean>) {
		return as_bool();
	} else if constexpr (std::integral<T>) {
		if constexpr (std::is_signed_v<T>) {
			return static_cast<T>(as_i64(static_cast<std::int64_t>(fallback)));
		} else {
			return static_cast<T>(as_u64(static_cast<std::uint64_t>(fallback)));
		}
	} else if constexpr (std::floating_point<T>) {
		return static_cast<T>(as_double(fallback));
	} else if constexpr (std::same_as<T, std::string>) {
		return std::string{as_string(std::string_view{fallback})};
	} else if constexpr (std::convertible_to<std::string_view, T>) {
		return static_cast<T>(as_string(std::string_view{fallback}));
	} else {
		return fallback;
	}
}

template <Primitive T>
void from_json(Json const& json, T& out, T const& fallback) {
	out = json.as<T>(fallback);
}

template <Primitive T>
void to_json(Json& out, T const& t) {
	if constexpr (std::same_as<T, bool>) {
		out = Json{Boolean{t}};
	} else {
		out = Json{t};
	}
}
} // namespace dj
