#include <algorithm>
#include <cassert>
#include <cerrno>
#include <map>
#include <sstream>
#include <stdexcept>
#include <detail/parser.hpp>
#include <dumb_json/djson.hpp>
#include <dumb_json/error_handler.hpp>

namespace dj {
namespace {
constexpr std::array g_trues = {"true", "True", "TRUE", "on", "On", "ON"};
constexpr std::array g_falses = {"false", "False", "FALSE", "off", "Off", "OFF"};

constexpr char peek(std::string_view text, std::size_t idx) noexcept { return idx < text.size() ? text[idx] : '\0'; }

bool is_bool(std::string_view str, bool match_empty) noexcept {
	if (str.empty()) { return match_empty; }
	auto const pred = [str](std::string_view const s) { return s == str; };
	return std::any_of(g_trues.begin(), g_trues.end(), pred) || std::any_of(g_falses.begin(), g_falses.end(), pred);
}

bool is_number(std::string_view str, bool match_empty) noexcept {
	if (str.empty()) { return match_empty; }
	if (str[0] == '-') { str = str.substr(1); }
	bool decimal = false;
	for (char const ch : str) {
		if (ch == '.') {
			if (decimal) { return false; }
			decimal = true;
			continue;
		}
		if (!std::isdigit(static_cast<unsigned char>(ch))) { return false; }
	}
	return true;
}

std::string unescape(std::string_view value) {
	std::string ret;
	ret.reserve(value.size());
	for (std::size_t idx = 0; idx < value.size(); ++idx) {
		if (peek(value, idx) == '\\') {
			if (++idx >= value.size()) { break; }
			switch (peek(value, idx)) {
			case '\\': break;
			case 'b':
				if (!ret.empty()) { ret.pop_back(); }
				++idx;
				break;
			case 't': ret += '\t'; break;
			default: break;
			}
		}
		if (char const ch = peek(value, idx); ch != '\0') { ret += ch; }
	}
	return ret;
}

template <typename T>
T& get_or_create(value_t& out) {
	auto p_t = std::get_if<T>(&out);
	if (!p_t) {
		out = T();
		p_t = std::get_if<T>(&out);
	}
	return *p_t;
}

void fill(node_t& out_root, detail::parser::node_t const& node) {
	using detail::nd_type;
	switch (node.payload.type) {
	case nd_type::scalar: {
		if (!node.children.empty()) {
			// scalar key
			assert(node.key);
			auto& fields = get_or_create<field_map_t>(out_root.m_value);
			auto [it, res] = fields.emplace(unescape(node.payload.text), node_t::make_ptr());
			assert(res);
			auto& value = *it->second;
			// header contains value(s) metadata
			auto& header = node.children[0];
			switch (header.payload.type) {
			case nd_type::scalar: value.m_value = scalar_t(header.payload.text); break;
			case nd_type::array:
			case nd_type::object: fill(value, header); break;
			default:
				// TODO error
				break;
			}
		} else {
			// array values
			auto& fields = get_or_create<field_array_t>(out_root.m_value);
			fields.push_back(node_t::make_ptr());
			fields.back()->m_value = scalar_t(node.payload.text);
		}
		break;
	}
	case nd_type::array: {
		// array header
		auto& fields = get_or_create<field_array_t>(out_root.m_value);
		for (auto const& child : node.children) {
			fields.push_back(node_t::make_ptr());
			auto& value = *fields.back();
			switch (child.payload.type) {
			case nd_type::object:
			case nd_type::array: fill(value, child); break;
			case nd_type::scalar: value.m_value = scalar_t(child.payload.text); break;
			default: break;
			}
		}
		break;
	}
	case nd_type::object: {
		// object header
		for (auto const& child : node.children) { fill(out_root, child); }
		break;
	}
	default:
		// TODO error
		break;
	}
}
} // namespace

namespace {
struct indenter final {
	serial_opts_t const* opts = nullptr;
	std::uint8_t indent = 0;
};
struct newliner final {
	serial_opts_t const* opts = nullptr;
};
struct spacer final {
	serial_opts_t const* opts = nullptr;
	std::uint8_t spaces = 1;
};
std::ostream& operator<<(std::ostream& out, indenter s) {
	while (s.opts->pretty && s.indent-- > 0) {
		if (s.opts->space_indent > 0) {
			for (auto i = s.opts->space_indent; i > 0; --i) { out << ' '; }
		} else {
			out << "\t";
		}
	}
	return out;
}
std::ostream& operator<<(std::ostream& out, newliner n) {
	if (n.opts->pretty) { out << n.opts->newline; }
	return out;
}
std::ostream& operator<<(std::ostream& out, spacer s) {
	while (s.opts->pretty && s.spaces-- > 0) { out << ' '; }
	return out;
}

std::string serialise_str(std::string_view str) {
	std::string ret;
	// bool const
	ret.reserve(str.size());
	for (char const c : str) {
		if (c == '"' || c == '\\') { ret += '\\'; }
		ret += c;
	}
	return ret;
}

std::ostream& serialise_scalar(std::ostream& out, std::string_view str) {
	if (str.empty()) {
		out << "\"\"";
	} else {
		bool const quoted = !is_bool(str, false) && !is_number(str, false);
		if (quoted) { out << "\""; }
		out << str;
		if (quoted) { out << "\""; }
	}
	return out;
}

std::ostream& serialise_node(serial_opts_t const& opts, std::ostream& out, std::uint8_t indent, node_t const& node, bool b_last = true);

std::ostream& serialise_object(field_map_t const& fields, std::ostream& out, serial_opts_t const& options, std::uint8_t indent) {
	if (fields.empty()) {
		out << "{}";
	} else {
		indenter t{&options, indent};
		newliner n{&options};
		spacer s{&options, 1};
		out << '{' << n;
		++t.indent;
		std::size_t index = 0;
		if (options.sort_keys) {
			std::map<std::string, node_t const*> map;
			for (auto const& [key, node] : fields) { map[serialise_str(key)] = node.get(); }
			for (auto const& [key, p_node] : map) {
				if (index > 0) { out << n; }
				out << t << '\"' << key << "\":" << s;
				serialise_node(options, out, t.indent, *p_node, (index == fields.size() - 1));
				++index;
			}
		} else {
			for (auto const& [key, node] : fields) {
				if (index > 0) { out << n; }
				out << t << '\"' << serialise_str(key) << "\":" << s;
				serialise_node(options, out, t.indent, *node, (index == fields.size() - 1));
				++index;
			}
		}
		--t.indent;
		out << n << t << '}';
	}
	return out;
}

std::ostream& serialise_array(field_array_t const& fields, std::ostream& out, serial_opts_t const& options, std::uint8_t indent) {
	if (fields.empty()) {
		out << "[]";
	} else {
		indenter t{&options, indent};
		newliner n{&options};
		out << '[' << n;
		++t.indent;
		std::size_t index = 0;
		for (auto const& node : fields) {
			if (index > 0) { out << n; }
			out << t;
			serialise_node(options, out, t.indent, *node, (index == fields.size() - 1));
			++index;
		}
		--t.indent;
		out << n << t << ']';
	}
	return out;
}

std::ostream& serialise_node(serial_opts_t const& opts, std::ostream& out, std::uint8_t indent, node_t const& node, bool b_last) {
	switch (node.type()) {
	case node_type::scalar: {
		serialise_scalar(out, std::get<scalar_t>(node.m_value));
		break;
	}
	case node_type::array: {
		serialise_array(std::get<field_array_t>(node.m_value), out, opts, indent);
		break;
	}
	case node_type::object: {
		serialise_object(std::get<field_map_t>(node.m_value), out, opts, indent);
		break;
	}
	default: break;
	}
	if (!b_last) { out << ','; }
	return out;
}

node_t& add_node(field_array_t& out_array, value_t&& value) {
	node_ptr node = node_t::make_ptr();
	node->m_value = std::move(value);
	out_array.push_back(node);
	return *node;
}

node_t& add_node(field_map_t& out_map, std::string_view key, value_t&& value) {
	node_ptr node = node_t::make_ptr();
	node->m_value = std::move(value);
	std::string k(key);
	if (auto it = out_map.find(k); it != out_map.end()) {
		it->second = node;
	} else {
		out_map.emplace(std::move(k), node);
	}
	return *node;
}
} // namespace

std::optional<node_t> node_t::make(std::string_view text) {
	if (auto const roots = read(text); !roots.empty()) { return roots.front(); }
	return std::nullopt;
}

node_ptr node_t::make_ptr(node_t* to_move) { return to_move ? std::make_shared<node_t>(std::move(*to_move)) : std::make_shared<node_t>(); }

std::vector<node_t> node_t::read(std::string_view text) {
	std::vector<node_t> ret;
	try {
		detail::parser prs;
		auto const ast = prs.parse(text);
		ret.reserve(ast.size());
		for (detail::parser::node_t const& node : ast) {
			node_t root;
			fill(root, node);
			ret.push_back(std::move(root));
		}
	} catch (parse_error_t const& e) {
		if constexpr (error_handler_t::action == error_handler_t::action_t::throw_ex) { throw e; }
	}
	return ret;
}

bool node_t::contains(std::string const& key) const {
	if (auto p_map = std::get_if<field_map_t>(&m_value)) { return p_map->find(key) != p_map->end(); }
	return false;
}

node_t& node_t::operator[](std::string const& key) { return sub(key); }

node_t& node_t::sub(std::string const& key) {
	auto p_map = std::get_if<field_map_t>(&m_value);
	if (!p_map) {
		m_value = field_map_t();
		p_map = std::get_if<field_map_t>(&m_value);
	}
	auto it = p_map->find(key);
	if (it == p_map->end()) {
		auto [i, r] = p_map->emplace(key, node_t::make_ptr());
		assert(r);
		it = i;
	}
	return *it->second;
}

node_t& node_t::get(std::string const& key) const {
	auto p_map = std::get_if<field_map_t>(&m_value);
	if (!p_map) { throw node_error(key); }
	auto it = p_map->find(key);
	if (it == p_map->end()) { throw node_error(key); }
	return *it->second;
}

node_t const& node_t::safe_get(std::string const& key) const {
	static node_t const blank;
	auto p_map = std::get_if<field_map_t>(&m_value);
	if (!p_map) { return blank; }
	auto it = p_map->find(key);
	if (it == p_map->end()) { return blank; }
	return *it->second;
}

node_t* node_t::find(std::string const& key) const {
	auto p_map = std::get_if<field_map_t>(&m_value);
	if (!p_map) { return nullptr; }
	auto it = p_map->find(key);
	if (it == p_map->end()) { return nullptr; }
	return it->second.get();
}

void node_t::set(value_t&& value) { m_value = std::move(value); }

void node_t::set(array_fields_t<value_t>&& values) {
	field_array_t fields;
	for (value_t& value : values) { add_node(fields, std::move(value)); }
	m_value = std::move(fields);
}

void node_t::set(map_fields_t<value_t>&& values) {
	field_map_t fields;
	for (auto& [key, value] : values) { add_node(fields, key, std::move(value)); }
	m_value = std::move(fields);
}

node_t& node_t::add(value_t&& value) {
	auto& fields = get_or_create<field_array_t>(m_value);
	return add_node(fields, std::move(value));
}

node_t& node_t::add(std::string_view key, value_t&& value) {
	auto& fields = get_or_create<field_map_t>(m_value);
	return add_node(fields, key, std::move(value));
}

void node_t::add(node_t&& node) {
	auto& fields = get_or_create<field_array_t>(m_value);
	fields.push_back(node_t::make_ptr(&node));
}

void node_t::add(std::string_view key, node_t&& node) {
	auto& fields = get_or_create<field_map_t>(m_value);
	std::string k(key);
	if (auto it = fields.find(k); it != fields.end()) {
		it->second = node_t::make_ptr(&node);
	} else {
		fields.emplace(std::move(k), node_t::make_ptr(&node));
	}
}

node_type node_t::type() const noexcept {
	if (std::holds_alternative<scalar_t>(m_value)) {
		return node_type::scalar;
	} else if (std::holds_alternative<field_map_t>(m_value)) {
		return node_type::object;
	} else if (std::holds_alternative<field_array_t>(m_value)) {
		return node_type::array;
	} else {
		return node_type::unknown;
	}
}

bool node_t::is_scalar() const noexcept { return type() == node_type::scalar; }

bool node_t::is_object() const noexcept { return type() == node_type::object; }

bool node_t::is_array() const noexcept { return type() == node_type::array; }

std::ostream& node_t::serialise(std::ostream& out, serial_opts_t const& opts, std::uint8_t indent) const { return serialise_node(opts, out, indent, *this); }

std::string node_t::to_string(serial_opts_t const& opts) const {
	std::stringstream str;
	serialise(str, opts, 0);
	return str.str();
}

std::string converter_t<std::string>::operator()(std::string_view scalar) const noexcept { return unescape(scalar); }

std::int64_t converter_t<std::int64_t>::operator()(std::string_view scalar) const noexcept { return std::atoll(scalar.data()); }

std::uint64_t converter_t<std::uint64_t>::operator()(std::string_view scalar) const noexcept {
	std::uint64_t ret;
	try {
		ret = static_cast<std::uint64_t>(std::stoull(scalar.data()));
	} catch (std::invalid_argument const&) {
		// TODO error
		ret = 0;
	}
	if (errno == ERANGE) {
		ret = 0;
		errno = 0;
	}
	return ret;
}

double converter_t<double>::operator()(std::string_view scalar) const noexcept {
	double ret;
	try {
		ret = std::stod(scalar.data());
	} catch (std::invalid_argument const&) {
		// TODO error
		ret = 0.0;
	}
	return ret;
}

bool converter_t<bool>::operator()(std::string_view scalar) const noexcept {
	return std::any_of(g_trues.begin(), g_trues.end(), [scalar](std::string_view str) { return str == scalar; });
}
} // namespace dj
