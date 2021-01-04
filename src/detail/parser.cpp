#include <algorithm>
#include <cassert>
#include <detail/diagnostics.hpp>
#include <detail/lexer.hpp>
#include <detail/parser.hpp>

namespace dj::detail {
namespace {
template <typename T>
constexpr bool any(T const& lhs, std::initializer_list<T> rhs) noexcept {
	return std::any_of(rhs.begin(), rhs.end(), [&lhs](T const& r) { return lhs == r; });
}

parser::node_t make_node(lexer& out_lexer) {
	parser::node_t node;
	if (auto token = out_lexer.advance()) {
		node.payload.loc = token->loc;
		if (token->type == tk_type::curly_open) {
			node.payload.type = nd_type::object;
			while (true) {
				auto const peek = out_lexer.peek();
				if (peek.empty()) {
					break;
				}
				token = peek[0];
				if (token->type == tk_type::curly_close) {
					break;
				}
				node.children.push_back(make_node(out_lexer));
				if (token = out_lexer.advance(); !token || token->type != tk_type::comma) {
					break;
				}
			}
			if (!detail::test_expect(token, out_lexer, tk_type::curly_close)) {
				return node;
			}
		} else if (token->type == tk_type::square_open) {
			node.payload.type = nd_type::array;
			while (true) {
				auto const peek = out_lexer.peek();
				if (peek.empty()) {
					break;
				}
				token = peek[0];
				if (token->type == tk_type::square_close) {
					break;
				}
				node.children.push_back(make_node(out_lexer));
				if (token = out_lexer.advance(); !token || token->type != tk_type::comma) {
					break;
				}
			}
			if (!detail::test_expect(token, out_lexer, tk_type::square_close)) {
				return node;
			}
		} else if (token->type == tk_type::value) {
			node.payload.type = nd_type::scalar;
			node.payload.text = token->text;
			auto const peek = out_lexer.peek();
			if (!peek.empty() && peek[0].type == tk_type::colon) {
				node.key = true;
				out_lexer.advance();
				node.children.push_back(make_node(out_lexer));
			}
		} else {
			detail::fail_unexpect(*token);
			out_lexer.advance();
		}
	}
	return node;
}
} // namespace

parser::ast parser::parse(std::string_view text) {
	ast ret;
	lexer lex(text);
	auto peek = lex.peek();
	while (!peek.empty()) {
		ret.push_back(make_node(lex));
		peek = lex.peek();
	}
	return ret;
}
} // namespace dj::detail
