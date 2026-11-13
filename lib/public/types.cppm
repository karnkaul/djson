export module djson.types;

export import std;

export namespace dj {
/// \brief Numeric type.
template <typename Type>
concept NumericT = std::integral<Type> || std::floating_point<Type>;

/// \brief Stringy type.
template <typename Type>
concept StringyT = std::convertible_to<Type, std::string_view>;

/// \brief Type to obtain JSON value as.
template <typename Type>
concept GettableT = std::same_as<Type, bool> || NumericT<Type> || StringyT<Type>;

/// \brief Type to set JSON value to.
template <typename Type>
concept SettableT = std::same_as<Type, std::nullptr_t> || GettableT<Type>;

/// \brief JSON value type.
enum class JsonType : std::int8_t { Null, Boolean, Number, String, Array, Object, COUNT_ };

/// \brief Source location.
struct SrcLoc {
	std::uint64_t line{};
	std::uint64_t column{};
};

/// \brief Various kinds of parse and IO errors.
/// Contains contextual token and source location if parse error.
struct Error {
	enum class Type : std::int8_t {
		Unknown,
		UnrecognizedToken,
		MissingClosingQuote,
		InvalidNumber,
		InvalidEscape,
		UnexpectedToken,
		UnexpectedComment,
		UnexpectedEof,
		MissingKey,
		MissingColon,
		MissingBrace,
		MissingBracket,
		MissingEndComment,
		IoError,
		UnsupportedFeature,
		COUNT_,
	};

	Type type{Type::Unknown};
	std::string token{};
	SrcLoc src_loc{};
};

/// \brief Bit flags for serialization options.
struct SerializeFlag {
	// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
	enum : std::uint8_t {
		None = 0,
		/// \brief Sort keys lexicographically.
		SortKeys = 1 << 0,
		/// \brief Append newline at the end. Ignored if NoSpaces is set.
		TrailingNewline = 1 << 1,
		/// \brief No whitespace. Ignores TrailingNewLine and other whitespace options.
		NoSpaces = 1 << 2,
	};
};
using SerializeFlags = decltype(std::to_underlying(SerializeFlag::None));
/// \brief Default serialize flags.
inline constexpr auto serialize_flags_v =
	static_cast<SerializeFlags>(SerializeFlag::SortKeys | SerializeFlag::TrailingNewline);

/// \brief Serialization options.
struct SerializeOptions {
	/// \brief Indentation string. Ignored if SerializeFlag::NoSpaces is set.
	std::string_view indent{"  "};
	/// \brief Newline string. Ignored if SerializeFlag::NoSpaces is set.
	std::string_view newline{"\n"};
	SerializeFlags flags{serialize_flags_v};
};

/// \brief Parse mode.
enum class ParseMode : std::int8_t {
	/// \brief Automatic: Strict unless first line specifies JSONC mode, or file extension is .jsonc.
	Auto,
	/// \brief Core JSON only.
	Strict,
	/// \brief Allow JSONC extensions.
	Jsonc,
};

/// \brief Obtain stringified Error Type.
auto to_string_view(Error::Type type) -> std::string_view;

/// \brief Obtain print-friendly error string.
auto to_string(Error const& error) -> std::string;

/// \brief Heterogeneous string hasher.
struct StringHash : std::hash<std::string_view> {
	using is_transparent = void;
};

/// \brief Heterogeneous string map.
template <typename Value>
using StringTable = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;

/// \brief Parse result type.
template <typename Type>
using Result = std::expected<Type, Error>;
} // namespace dj

export namespace dj::detail {
class Polymorphic {
  public:
	Polymorphic() = default;
	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	Polymorphic& operator=(Polymorphic const&) = default;
	Polymorphic& operator=(Polymorphic&&) = default;

	virtual ~Polymorphic() = default;
};
} // namespace dj::detail
