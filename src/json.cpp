#include <detail/parser.hpp>
#include <djson/json.hpp>
#include <fstream>
#include <iomanip>

namespace dj {
namespace detail {
namespace {
struct formatter {
	std::ostream& out;
	int indent{};
	bool pretty{true};

	formatter& space() {
		if (pretty) { out << ' '; }
		return *this;
	}

	formatter& next_line(bool comma, bool deindent) {
		if (comma) { out << ','; }
		if (deindent && indent > 0) { --indent; }
		if (pretty) {
			out << '\n';
			for (int i = 0; i < indent; ++i) { out << '\t'; }
		}
		return *this;
	}

	template <typename T>
	friend formatter& operator<<(formatter& in, T const& t) {
		in.out << t;
		return in;
	}
};
} // namespace

struct escape {
	std::string_view text{};
};

formatter& operator<<(formatter& str, escape text) {
	str << '\"';
	for (char const ch : text.text) {
		switch (ch) {
		case '\n': str << R"(\n)"; continue;
		case '\t': str << R"(\t)"; continue;
		case '\\': str << R"(\\)"; continue;
		case '\"': str << R"(\")"; continue;
		default: break;
		}
		str << ch;
	}
	return str << '\"';
}

template <>
struct facade<formatter> {
	void write_unquoted(formatter& out, json const& js) const { out << std::get<lit_t>(js.m_value.value); }
	void write_quoted(formatter& out, json const& js) const { out << escape{std::get<lit_t>(js.m_value.value)}; }

	void unquoted(formatter& out, json const& js, bool multi) const {
		write_unquoted(out, js);
		out.next_line(multi, !multi);
	}

	void quoted(formatter& out, json const& js, bool multi) const {
		write_quoted(out, js);
		out.next_line(multi, !multi);
	}

	void array(formatter& out, json const& js, bool multi) const {
		out << '[';
		auto const& arr = std::get<arr_t>(js.m_value.value);
		if (!arr.nodes.empty()) {
			++out.indent;
			out.next_line(false, false);
			for (std::size_t i = 0; i < arr.nodes.size(); ++i) { write(out, *arr.nodes[i], i + 1 < arr.nodes.size()); }
		}
		out << ']';
		out.next_line(multi, !multi);
	}

	void object(formatter& out, json const& js, bool multi) const {
		out << '{';
		auto const& obj = std::get<obj_t>(js.m_value.value);
		if (!obj.nodes.empty()) {
			++out.indent;
			out.next_line(false, false);
			auto const size = obj.nodes.size();
			for (std::size_t i = 0; i < size; ++i) {
				auto const& entry = obj.nodes[i];
				out << escape{entry.first} << ':';
				out.space();
				write(out, *obj.nodes[i].second, i + 1 < obj.nodes.size());
			}
		}
		out << '}';
		out.next_line(multi, !multi);
	}

	void write(formatter& out, json const& js, bool multi) const {
		switch (js.type()) {
		case value_type::null:
		case value_type::boolean:
		case value_type::number: unquoted(out, js, multi); break;
		case value_type::string: quoted(out, js, multi); break;
		case value_type::array: array(out, js, multi); break;
		case value_type::object: object(out, js, multi);
		default: break;
		}
	}

	formatter& operator()(formatter& out, json const& js) const {
		write(out, js, false);
		return out;
	}
};

auto fifo_map_t::insert_or_assign(std::string const& key, ujson value) -> std::pair<iterator, bool> {
	if (auto it = indices.find(key); it != indices.end()) {
		entries[it->second].second = std::move(value);
		return {entries.begin() + it->second, false};
	} else {
		indices.emplace(key, entries.size());
		entries.push_back({key, std::move(value)});
		return {entries.end() - 1, true};
	}
}

auto fifo_map_t::find(std::string const& key) const -> const_iterator {
	if (auto it = indices.find(key); it != indices.end()) { return entries.begin() + static_cast<std::ptrdiff_t>(it->second); }
	return entries.end();
}

ujson& fifo_map_t::operator[](std::string const& key) {
	if (auto it = indices.find(key); it != indices.end()) { return entries[it->second].second; }
	indices.emplace(key, entries.size());
	entries.push_back({key, ujson{}});
	return entries.back().second;
}

value_t::value_t(val_t value, value_type type) noexcept : value(std::move(value)), type(type) {}
value_t::value_t(value_t const& rhs) : value(clone(rhs.value)), type(rhs.type) {}
value_t& value_t::operator=(value_t const& rhs) {
	if (&rhs != this) {
		value = clone(rhs.value);
		type = rhs.type;
	}
	return *this;
}

parse_result make_error(detail::error_t err) { return {std::move(err.text), err.loc.line, err.loc.col_index + 1}; }

parse_result set_value(std::string_view text, value_t& out) {
	auto parser = parser_t{text};
	try {
		out = parser.parse();
	} catch (detail::error_t const& error) { return make_error(error); }
	return {};
}

template <>
struct facade<array_view> {
	arr_t const* operator()(json const& json) const { return std::get_if<arr_t>(&json.m_value.value); }
};

template <>
struct facade<object_view> {
	obj_t const* operator()(json const& json) const { return std::get_if<obj_t>(&json.m_value.value); }
};
} // namespace detail

static auto const empty_v = json{};

parse_result json::read(std::string_view const text) { return set_value(text, m_value); }

io_result json::open(char const* path) {
	auto ret = io_result{};
	auto file = std::ifstream(path, std::ios::in | std::ios::ate);
	if (!file) {
		ret.file_path = path;
		return ret;
	}
	auto str = std::string{};
	auto const size = file.tellg();
	str.resize(static_cast<std::size_t>(size));
	file.seekg({});
	file.read(str.data(), size);
	if (str.empty()) {
		ret.file_path = path;
		return ret;
	}
	auto res = read(str);
	if (res) {
		static_cast<parse_result&>(ret) = std::move(res);
		ret.file_path = path;
		ret.file_contents = std::move(str);
		return ret;
	}
	return ret;
}

bool json::contains(std::string const& key) const {
	if (auto obj = std::get_if<detail::obj_t>(&m_value.value)) { return obj->nodes.indices.contains(key); }
	return false;
}

json& json::value(std::string const& key) {
	if (!std::holds_alternative<detail::obj_t>(m_value.value)) { m_value.value = detail::obj_t{}; }
	auto& ret = std::get<detail::obj_t>(m_value.value).nodes[key];
	if (!ret) { ret = std::make_unique<json>(); }
	return *ret;
}

json const& json::value(std::string const& key) const {
	if (!std::holds_alternative<detail::obj_t>(m_value.value)) { return empty_v; }
	auto const& map = std::get<detail::obj_t>(m_value.value).nodes;
	auto const it = map.find(key);
	if (it == map.end() || !it->second) { return empty_v; }
	return *it->second;
}

std::size_t json::container_size() const {
	if (auto obj = std::get_if<detail::obj_t>(&m_value.value)) {
		return obj->nodes.size();
	} else if (auto arr = std::get_if<detail::arr_t>(&m_value.value)) {
		return arr->nodes.size();
	}
	return {};
}

bool json::as_boolean(bool fallback) const { return detail::facade<bool>{}(*this, fallback); }

std::string json::as_string(std::string const& fallback) const { return detail::facade<std::string>{}(*this, fallback); }

std::string_view json::as_string_view(std::string_view fallback) const { return detail::facade<std::string_view>{}(*this, fallback); }

std::string json::serialize(bool pretty_print) const {
	auto str = std::stringstream{};
	auto fmt = detail::formatter{str, 0, pretty_print};
	detail::facade<decltype(fmt)>{}(fmt, *this);
	return str.str();
}

bool json::write(char const* path, bool pretty_print) const { return serializer{*this, pretty_print}(path); }

std::string minify(std::string_view text) {
	auto str = std::stringstream{};
	for (char const ch : text) {
		if (!detail::scanner_t::is_space(ch)) { str << ch; }
	}
	return str.str();
}

std::ostream& operator<<(std::ostream& out, serializer const& wr) {
	auto fmt = detail::formatter{out, 0, wr.pretty_print};
	return detail::facade<decltype(fmt)>{}(fmt, wr.doc).out;
}

std::ostream& operator<<(std::ostream& out, parse_result const& result) {
	if (!result) { out << "[djson] Unexpected token [" << result.token << "] at line " << result.line << " col " << result.column; }
	return out;
}

std::ostream& operator<<(std::ostream& out, io_result const& result) {
	if (!result.file_path.empty()) {
		out << "[djson] Failed to read file contents [" << result.file_path << "]";
	} else {
		out << static_cast<parse_result>(result);
	}
	return out;
}

bool serializer::operator()(char const* path) const {
	auto file = std::ofstream(path);
	if (!file) { return false; }
	file << *this;
	return true;
}

std::string_view detail::facade<std::string_view>::operator()(json const& js, std::string_view fallback) const {
	if (!js.is_string()) { return fallback; }
	if (auto str = std::get_if<std::string>(&js.m_value.value)) { return *str; }
	return fallback;
}

std::string detail::facade<std::string>::operator()(json const& js, std::string const& fallback) const {
	if (!js.is_string()) { return fallback; }
	if (auto str = std::get_if<std::string>(&js.m_value.value)) { return *str; }
	return fallback;
}

bool detail::facade<bool>::operator()(json const& js, bool fallback) const {
	if (!js.is_boolean()) { return fallback; }
	return std::get<std::string>(js.m_value.value) == "true";
}

array_view::array_view(json const& json) : m_value(detail::facade<array_view>{}(json)) {}

json const& array_view::operator[](std::size_t index) const {
	if (!m_value || index >= m_value->nodes.size()) { return empty_v; }
	auto& ret = m_value->nodes[index];
	if (!ret) { return empty_v; }
	return *ret;
}

object_view::object_view(json const& json) : m_value(detail::facade<object_view>{}(json)) {}
} // namespace dj
