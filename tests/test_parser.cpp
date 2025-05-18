#include <detail/parser.hpp>
#include <unit_test.hpp>
#include <cmath>
#include <format>
#include <limits>
#include <print>

namespace {
using namespace dj;

using ErrType = Error::Type;

[[nodiscard]] auto expect_json(std::string_view const text, ParseFlags const flags = {}) {
	auto parser = detail::Parser{text, flags};
	auto result = parser.parse();
	ASSERT(result);
	return *result;
}

[[nodiscard]] auto expect_error(std::string_view const text, ParseFlags const flags = {}) {
	auto parser = detail::Parser{text, flags};
	auto result = parser.parse();
	ASSERT(!result);
	return result.error();
}

TEST(parser_null) {
	auto json = expect_json("");
	EXPECT(json.is_null());
	json = expect_json("null");
	EXPECT(json.is_null());
}

TEST(parser_boolean) {
	auto json = expect_json("true");
	EXPECT(json.is_boolean());
	EXPECT(json.as_bool());

	json = expect_json("false");
	EXPECT(json.is_boolean());
	EXPECT(!json.as<bool>());

	// fallback
	json = expect_json("1");
	EXPECT(!json.is_boolean());
	EXPECT(json.as_bool(true));
}

TEST(parser_number) {
	auto json = expect_json("123");
	EXPECT(json.is_number());
	EXPECT(json.as<int>() == 123);

	json = expect_json("-42");
	EXPECT(json.is_number());
	EXPECT(json.as_number<int>() == -42);

	json = expect_json("-3.14");
	EXPECT(json.is_number());
	EXPECT(std::abs(json.as<double>() - -3.14) < 0.001);

	json = expect_json("2.5e3");
	EXPECT(json.is_number());
	EXPECT(std::abs(json.as<double>() - 2500) < 0.001);

	static constexpr auto ularge_v = std::numeric_limits<std::uint64_t>::max() - 2;
	auto str = std::format("{}", ularge_v);
	json = expect_json(str);
	EXPECT(json.is_number());
	EXPECT(json.as_u64() == ularge_v);

	static constexpr auto ilarge_v = std::numeric_limits<std::int64_t>::min();
	str.clear();
	std::format_to(std::back_inserter(str), "{}", ilarge_v);
	json = expect_json(str);
	EXPECT(json.is_number());
	EXPECT(json.as_i64() == ilarge_v);
}

TEST(parser_string) {
	auto json = expect_json(R"("")");
	EXPECT(json.is_string());
	EXPECT(json.as_string_view().empty());

	json = expect_json(R"("hello world"
// comment
)");
	EXPECT(json.is_string());
	EXPECT(json.as<std::string_view>() == "hello world");

	json = expect_json(R"("exx\btra\nnewline\\\/\"")");
	EXPECT(json.is_string());
	EXPECT(json.as_string_view() == R"(extra
newline\/")");
}

TEST(parser_array) {
	auto json = expect_json("[]");
	EXPECT(json.is_array());
	EXPECT(json.as_array().empty());

	json = expect_json("[3, 5, 8]");
	EXPECT(json.is_array());
	EXPECT(json.as_array().size() == 3);
	EXPECT(json[0].as<int>() == 3);
	EXPECT(json[1].as<int>() == 5);
	EXPECT(json[2].as<int>() == 8);

	json = expect_json(R"([-42, {}, "hi"])");
	EXPECT(json.as_array().size() == 3);
	EXPECT(json[0].as<int>() == -42);
	EXPECT(json[1].is_object());
	EXPECT(json[1].as_object().empty());
	EXPECT(json[2].is_string());
	EXPECT(json[2].as_string_view() == "hi");
}

TEST(parser_object) {
	auto json = expect_json("{}");
	EXPECT(json.is_object());
	EXPECT(json.as_object().empty());

	json = expect_json(R"({
	"foo": "bar",
	"pi": 3.14
})");
	EXPECT(json.is_object());
	EXPECT(json.as_object().size() == 2);
	EXPECT(json["foo"].as_string_view() == "bar");
	EXPECT(json["pi"].as<double>() == 3.14);
}

TEST(parser_unrecognized_token) {
	auto error = expect_error(R"([ true, 42, $ ])");
	EXPECT(error.type == ErrType::UnrecognizedToken);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 13);
	EXPECT(error.token == R"($)");
	std::println("{}", to_string(error));
}

TEST(parser_missing_quote) {
	auto error = expect_error(R"("hi)");
	EXPECT(error.type == ErrType::MissingClosingQuote);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 1);
	EXPECT(error.token == R"(")");
	std::println("{}", to_string(error));
}

TEST(parser_unexpected_token) {
	auto error = expect_error(R"(42 true)");
	EXPECT(error.type == ErrType::UnexpectedToken);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 4);
	EXPECT(error.token == "true");
	std::println("{}", to_string(error));
}

TEST(parser_unexpected_comment) {
	auto error = expect_error(R"(	42 
// unexpected comment
true)",
							  ParseFlag::NoComments);
	EXPECT(error.type == ErrType::UnexpectedComment);
	EXPECT(error.src_loc.line == 2 && error.src_loc.column == 1);
	EXPECT(error.token == "// unexpected comment");
	std::println("{}", to_string(error));
}

TEST(parser_unexpected_eof) {
	auto error = expect_error(R"({"foo":)");
	EXPECT(error.type == ErrType::UnexpectedEof);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 8);
	EXPECT(error.token.empty());
	std::println("{}", to_string(error));
}

TEST(parser_invalid_number) {
	auto error = expect_error(R"(1.2e4e5)");
	EXPECT(error.type == ErrType::InvalidNumber);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 1);
	EXPECT(error.token == "1.2e4e5");
	std::println("{}", to_string(error));
}

TEST(parser_invalid_escape) {
	auto error = expect_error(R"("\xbar")");
	EXPECT(error.type == ErrType::InvalidEscape);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 1);
	EXPECT(error.token == R"("\xbar")");
	std::println("{}", to_string(error));
}

TEST(parser_missing_key) {
	auto error = expect_error(R"({42})");
	EXPECT(error.type == ErrType::MissingKey);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 2);
	EXPECT(error.token == "42");
	std::println("{}", to_string(error));
}

TEST(parser_missing_bracket) {
	auto error = expect_error(R"([0, 1, 2 {"foo": "bar})");
	EXPECT(error.type == ErrType::MissingBracket);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 10);
	EXPECT(error.token == "{");
	std::println("{}", to_string(error));
}

TEST(parser_missing_colon) {
	auto error = expect_error(R"({"foo" "bar"})");
	EXPECT(error.type == ErrType::MissingColon);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 8);
	EXPECT(error.token == R"("bar")");
	std::println("{}", to_string(error));
}

TEST(parser_missing_brace) {
	auto error = expect_error(R"({"foo": "bar")");
	EXPECT(error.type == ErrType::MissingBrace);
	EXPECT(error.src_loc.line == 1 && error.src_loc.column == 14);
	EXPECT(error.token.empty());
	std::println("{}", to_string(error));
}
} // namespace
