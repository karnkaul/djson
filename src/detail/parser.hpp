#pragma once
#include <vector>
#include <detail/common.hpp>

namespace dj::detail {
///
/// \brief AST schema: a "root" (document) is an N-tree of node_t objects
/// node_t models both keys and values:
/// - all key nodes are scalars (strings): payload => key, first child => header
/// - header nodes describe values
/// - scalar headers: payload (inline) => value
/// - array headers: children => values
/// - object headers: children => keys, children.children => values
///
class parser {
  public:
	struct node_t {
		payload_t payload;
		std::vector<node_t> children;
		bool key = false;
	};

	using ast = std::vector<node_t>;

	ast parse(std::string_view text);

  private:
};
} // namespace dj::detail
