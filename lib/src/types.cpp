module djson.types;

namespace dj {
using namespace std::string_view_literals;

namespace {

constexpr auto error_type_str_v = std::array{
	"Unknown error"sv,
	"Unrecognized Token"sv,
	"Missing closing quote"sv,
	"Invalid number"sv,
	"Invalid escape"sv,
	"Unexpected token"sv,
	"Unexpected comment"sv,
	"Unexpected end of file"sv,
	"Missing key"sv,
	"Missing colon (':')"sv,
	"Missing closing brace ('}')"sv,
	"Missing closing square bracket (']')"sv,
	"Missing end comment ('*/')"sv,
	"I/O error"sv,
	"Unsupported feature"sv,
};

static_assert(error_type_str_v.size() == static_cast<std::size_t>(Error::Type::COUNT_));
} // namespace
} // namespace dj

auto dj::to_string_view(Error::Type const type) -> std::string_view {
	if (static_cast<int>(type) < 0 || type >= Error::Type::COUNT_) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		return dj::error_type_str_v[static_cast<std::size_t>(Error::Type::Unknown)];
	}
	return dj::error_type_str_v.at(static_cast<std::size_t>(type));
}

auto dj::to_string(Error const& error) -> std::string {
	auto ret = std::string{to_string_view(error.type)};
	if (!error.token.empty()) {
		std::format_to(std::back_inserter(ret), " - '{}'", error.token);
	}
	auto const src_loc = error.src_loc;
	if (src_loc.line > 0 && src_loc.column > 0) {
		std::format_to(std::back_inserter(ret), " [{}:{}]", src_loc.line, src_loc.column);
	}
	return ret;
}
