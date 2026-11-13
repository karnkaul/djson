export module djson;

export import djson.types;
export import djson.build_version;

export namespace dj {
class Json;

namespace detail {
class IValue : public Polymorphic {};

class IParser : public Polymorphic {
  protected:
	[[nodiscard]] static auto create(std::unique_ptr<IValue> value) -> Json;
};
} // namespace detail

/// \brief Library interface, represents a valid JSON value.
class Json {
  public:
	using Type = JsonType;

	Json() = default;
	~Json() = default;

	Json(Json&&) = default;
	auto operator=(Json&&) -> Json& = default;

	Json(Json const& other);
	auto operator=(Json const& other) -> Json&;

	explicit(false) Json(std::nullptr_t) noexcept {}

	template <SettableT Type>
	explicit(false) Json(Type const& value) {
		set(value);
	}

	/// \brief Parse JSON text.
	/// \param text Input JSON text.
	/// \param mode Parse mode.
	/// \returns Json if successful, else Error.
	[[nodiscard]] static auto parse(std::string_view text, ParseMode mode = ParseMode::Auto) -> Result<Json>;
	/// \brief Parse JSON from a file.
	/// \param path Path to JSON file.
	/// \param mode Parse mode.
	/// \returns Json if successful, else Error.
	[[nodiscard]] static auto from_file(std::string_view path, ParseMode mode = ParseMode::Auto) -> Result<Json>;

	/// \brief Obtain a Json representing an empty Array value.
	[[nodiscard]] static auto empty_array() -> Json const&;
	/// \brief Obtain a Json representing an empty Object value.
	[[nodiscard]] static auto empty_object() -> Json const&;

	/// \brief Obtain the value type of this Json.
	[[nodiscard]] auto get_type() const -> Type;

	[[nodiscard]] auto is_null() const -> bool { return get_type() == Type::Null; }
	[[nodiscard]] auto is_boolean() const -> bool { return get_type() == Type::Boolean; }
	[[nodiscard]] auto is_number() const -> bool { return get_type() == Type::Number; }
	[[nodiscard]] auto is_string() const -> bool { return get_type() == Type::String; }
	[[nodiscard]] auto is_array() const -> bool { return get_type() == Type::Array; }
	[[nodiscard]] auto is_object() const -> bool { return get_type() == Type::Object; }

	[[nodiscard]] auto as_bool(bool fallback = {}) const -> bool;
	[[nodiscard]] auto as_double(double fallback = {}) const -> double;
	[[nodiscard]] auto as_u64(std::uint64_t fallback = {}) const -> std::uint64_t;
	[[nodiscard]] auto as_i64(std::int64_t fallback = {}) const -> std::int64_t;

	[[nodiscard]] auto as_string_view(std::string_view fallback = {}) const -> std::string_view;
	[[nodiscard]] auto as_string(std::string_view const fallback = {}) const -> std::string {
		return std::string{as_string_view(fallback)};
	}

	template <NumericT Type>
	[[nodiscard]] auto as_number(Type const fallback = {}) const -> Type {
		if constexpr (std::signed_integral<Type>) {
			return static_cast<Type>(as_i64(static_cast<std::int64_t>(fallback)));
		} else if constexpr (std::unsigned_integral<Type>) {
			return static_cast<Type>(as_u64(static_cast<std::uint64_t>(fallback)));
		} else {
			return static_cast<Type>(as_double(static_cast<double>(fallback)));
		}
	}

	template <GettableT Type>
	[[nodiscard]] auto as(Type const& fallback = {}) const -> Type {
		if constexpr (std::same_as<Type, bool>) {
			return as_bool(fallback);
		} else if constexpr (NumericT<Type>) {
			return as_number(fallback);
		} else if constexpr (std::same_as<Type, std::string_view>) {
			return as_string_view(fallback);
		} else {
			return as_string(fallback);
		}
	}

	[[nodiscard]] auto as_array() const -> std::span<dj::Json const>;
	[[nodiscard]] auto as_object() const -> StringTable<dj::Json> const&;

	void set_null();
	void set_boolean(bool value);
	void set_string(std::string_view value);
	void set_number(std::int64_t value);
	void set_number(std::uint64_t value);
	void set_number(double value);
	void set_value(Json value);

	template <NumericT Type>
	void set_number(Type const value) {
		if constexpr (std::signed_integral<Type>) {
			set_number(static_cast<std::int64_t>(value));
		} else if constexpr (std::unsigned_integral<Type>) {
			set_number(static_cast<std::uint64_t>(value));
		} else {
			set_number(static_cast<double>(value));
		}
	}

	template <SettableT Type>
	void set(Type const& value) {
		if constexpr (std::same_as<Type, std::nullptr_t>) {
			set_null();
		} else if constexpr (std::same_as<Type, bool>) {
			set_boolean(value);
		} else if constexpr (NumericT<Type>) {
			set_number(value);
		} else {
			set_string(value);
		}
	}

	/// \brief Set value to empty Array.
	void set_array();
	/// \brief Set value to empty Object.
	void set_object();

	/// \brief Insert value at the end of the Array.
	/// Converts to empty Array value first if not already one.
	/// \param value Value to insert.
	/// \returns Reference to inserted value.
	auto push_back(Json value = {}) -> Json&;
	/// \brief Insert value associated with key into the Object.
	/// Converts to empty Object value first if not already one.
	/// \param key Key to associate value with.
	/// \param value Value to insert.
	/// \returns Reference to inserted value.
	auto insert_or_assign(std::string key, Json value) -> Json&;

	/// \brief Obtain the value associated with the passed key.
	/// \param key Key to lookup value for.
	/// \returns Value if type is Object and key exists, else null.
	[[nodiscard]] auto at(std::string_view key) const -> Json const&;
	/// \brief Obtain the value associated with the passed key.
	/// \param key Key to lookup value for.
	/// \returns Reference to value if key exists, else newly inserted null value.
	[[nodiscard]] auto at(std::string_view key) -> Json&;
	/// \brief Obtain the value at the passed index.
	/// \param index Index to access value for.
	/// \returns Value at index if type is Array and index is less than size, else null.
	[[nodiscard]] auto at(std::size_t index) const -> Json const&;
	/// \brief Obtain the value at the passed index.
	/// \param index Index to access value for.
	/// \returns Reference to value at index. Resizes if necessary.
	[[nodiscard]] auto at(std::size_t index) -> Json&;

	[[nodiscard]] auto& operator[](this auto&& self, std::string_view const key) { return self.at(key); }
	[[nodiscard]] auto& operator[](this auto&& self, std::size_t const index) { return self.at(index); }

	/// \brief Serialize value as a string.
	/// \param options Serialization options.
	/// \returns Serialized string.
	[[nodiscard]] auto serialize(SerializeOptions const& options = {}) const -> std::string;
	/// \brief Write serialized string to a file.
	/// \param path Path to write to.
	/// \param options Serialization options.
	/// \returns true if file successfully written.
	[[nodiscard]] auto to_file(std::string_view path, SerializeOptions const& options = {}) const -> bool;

	friend void swap(Json& a, Json& b) noexcept { std::swap(a.m_value, b.m_value); }

	explicit operator bool() const { return m_value != nullptr; }

  private:
	struct Serializer;

	void ensure_impl();

	std::unique_ptr<detail::IValue> m_value;

	friend class detail::IParser;
};

[[nodiscard]] auto to_string(Json const& json, SerializeOptions const& options = {}) { return json.serialize(options); }

/// \brief Convert input text to escaped string.
[[nodiscard]] auto make_escaped(std::string_view text) -> std::string;

/// \brief Assign JSON as value.
template <GettableT Type>
void from_json(Json const& json, Type& value, Type const fallback = {}) {
	value = json.as<Type>(fallback);
}

/// \brief Assign value to JSON.
template <SettableT Type>
void to_json(Json& json, Type const& value) {
	json.set(value);
}
} // namespace dj

/// \brief Specialization for std::format (and related).
template <>
struct std::formatter<dj::Json> {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}

	static auto format(dj::Json const& json, std::format_context& fc) {
		auto const str = json.serialize(dj::SerializeOptions{.flags = dj::SerializeFlag::NoSpaces});
		return format_to(fc.out(), "{}", str);
	}
};
