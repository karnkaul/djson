#include <detail/scanner.hpp>

namespace {
using namespace dj::detail;
using Op = token::Operator;
using ErrType = ScanError::Type;

struct Fixture : Scanner {
	using Scanner::Scanner;

	constexpr auto expect_token() -> bool {
		auto result = next();
		if (!result) { return false; }
		token = *result;
		return true;
	}

	constexpr auto expect_eof() -> bool {
		if (!expect_token()) { return false; }
		return token.is<token::Eof>();
	}

	[[nodiscard]] constexpr auto expect_operator(Op const op) -> bool {
		if (!expect_token()) { return false; }
		if (!token.is<Op>()) { return false; }
		return std::get<Op>(token.type) == op;
	}

	constexpr auto expect_string(std::string_view const escaped) -> bool {
		if (!expect_token()) { return false; }
		if (!token.is<token::String>()) { return false; }
		return std::get<token::String>(token.type).escaped == escaped;
	}

	constexpr auto expect_number(std::string_view const raw_str) -> bool {
		if (!expect_token()) { return false; }
		if (!token.is<token::Number>()) { return false; }
		return std::get<token::Number>(token.type).raw_str == raw_str;
	}

	constexpr auto expect_error(ErrType const type, std::string_view const lexeme, std::uint64_t line, std::uint64_t column) {
		auto result = next();
		if (result) { return false; }
		auto err = result.error();
		if (err.type != type) { return false; }
		if (err.token != lexeme) { return false; }
		return err.src_loc.line == line && err.src_loc.column == column;
	}

	Token token{};
};

constexpr auto test_operators() {
	auto fixture = Fixture{" null	true,false :[ ] {}"};
	if (!fixture.expect_operator(Op::Null)) { return false; }
	if (!fixture.expect_operator(Op::True)) { return false; }
	if (!fixture.expect_operator(Op::Comma)) { return false; }
	if (!fixture.expect_operator(Op::False)) { return false; }
	if (!fixture.expect_operator(Op::Colon)) { return false; }
	if (!fixture.expect_operator(Op::SquareLeft)) { return false; }
	if (!fixture.expect_operator(Op::SquareRight)) { return false; }
	if (!fixture.expect_operator(Op::BraceLeft)) { return false; }
	if (!fixture.expect_operator(Op::BraceRight)) { return false; }
	return fixture.expect_eof();
}

constexpr auto test_strings() {
	auto fixture = Fixture{R"("hello" "\"world\"" "\\")"};
	if (!fixture.expect_string("hello")) { return false; }
	if (!fixture.expect_string(R"(\"world\")")) { return false; }
	if (!fixture.expect_string(R"(\\)")) { return false; }
	return fixture.expect_eof();
}

constexpr auto test_numbers() {
	auto fixture = Fixture{R"(42 3.14 1.234e-56)"};
	if (!fixture.expect_number("42")) { return false; }
	if (!fixture.expect_number("3.14")) { return false; }
	if (!fixture.expect_number("1.234e-56")) { return false; }
	return fixture.expect_eof();
}

constexpr auto test_comment() {
	auto fixture = Fixture{R"("hello"
// this is a comment
42)"};
	if (!fixture.expect_string("hello")) { return false; }
	if (!fixture.expect_token()) { return false; }
	if (!fixture.token.is<token::Comment>() || fixture.token.lexeme != "// this is a comment") { return false; }
	if (!fixture.expect_number("42")) { return false; }
	return fixture.expect_eof();
}

constexpr auto test_unrecognized_token() {
	auto fixture = Fixture{R"("hello"$)"};
	if (!fixture.expect_string("hello")) { return false; }
	return fixture.expect_error(ErrType::UnrecognizedToken, "$", 1, 8);
}

constexpr auto test_missing_quote() {
	auto fixture = Fixture{R"(42
"abc)"};
	if (!fixture.expect_number("42")) { return false; }
	return fixture.expect_error(ErrType::MissingClosingQuote, "\"", 2, 1);
}

static_assert(test_operators());
static_assert(test_strings());
static_assert(test_numbers());
static_assert(test_comment());
static_assert(test_unrecognized_token());
static_assert(test_missing_quote());
} // namespace
