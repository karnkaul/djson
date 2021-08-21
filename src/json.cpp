#include <fstream>
#include <map>
#include <detail/parser.hpp>
#include <dumb_json/json.hpp>

#include <iostream>

namespace dj {
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

std::string escape(std::string_view str) {
	std::string ret;
	ret.reserve(str.size());
	for (char const c : str) {
		if (c == '"' || c == '\\') { ret += '\\'; }
		ret += c;
	}
	return ret;
}

std::ostream& serialize_string(std::ostream& out, std::string_view str, bool quoted) {
	if (str.empty()) {
		out << "\"\"";
	} else {
		if (quoted) {
			out << "\"" << escape(str) << "\"";
		} else {
			out << str;
		}
	}
	return out;
}

std::ostream& serialize_json(json const& json, serial_opts_t const& opts, std::ostream& out, std::uint8_t indent, bool last);

std::ostream& serialize_object(object_t::storage_t const& fields, std::ostream& out, serial_opts_t const& options, std::uint8_t indent) {
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
			std::map<std::string, json const*> map;
			for (auto const& [key, json] : fields) { map[escape(key)] = json.get(); }
			for (auto const& [key, json] : map) {
				if (index > 0) { out << n; }
				out << t << '\"' << key << "\":" << s;
				serialize_json(*json, options, out, t.indent, (index == fields.size() - 1));
				++index;
			}
		} else {
			for (auto const& [key, json] : fields) {
				if (index > 0) { out << n; }
				out << t << '\"' << escape(key) << "\":" << s;
				serialize_json(*json, options, out, t.indent, (index == fields.size() - 1));
				++index;
			}
		}
		--t.indent;
		out << n << t << '}';
	}
	return out;
}

std::ostream& serialize_array(array_t::storage_t const& fields, std::ostream& out, serial_opts_t const& options, std::uint8_t indent) {
	if (fields.empty()) {
		out << "[]";
	} else {
		indenter t{&options, indent};
		newliner n{&options};
		out << '[' << n;
		++t.indent;
		std::size_t index = 0;
		for (auto const& json : fields) {
			if (index > 0) { out << n; }
			out << t;
			serialize_json(*json, options, out, t.indent, (index == fields.size() - 1));
			++index;
		}
		--t.indent;
		out << n << t << ']';
	}
	return out;
}

std::ostream& serialize_json(json const& json, serial_opts_t const& opts, std::ostream& out, std::uint8_t indent, bool last) {
	switch (json.type()) {
	case json_type::null: out << "null"; break;
	case json_type::boolean: out << (std::get<boolean_t>(json.value()).value ? "true" : "false"); break;
	case json_type::number: serialize_string(out, std::get<number_t>(json.value()).value, false); break;
	case json_type::string: serialize_string(out, std::get<string_t>(json.value()).value, true); break;
	case json_type::array: serialize_array(std::get<array_t>(json.value()).value, out, opts, indent); break;
	case json_type::object: serialize_object(std::get<object_t>(json.value()).value, out, opts, indent); break;
	}
	if (!last) { out << ','; }
	return out;
}
} // namespace

std::string json::result_t::to_string() const {
	std::stringstream str;
	str << *this;
	return str.str();
}

json_type json::type() const noexcept {
	if (std::holds_alternative<boolean_t>(m_value)) {
		return json_type::boolean;
	} else if (std::holds_alternative<number_t>(m_value)) {
		return json_type::number;
	} else if (std::holds_alternative<string_t>(m_value)) {
		return json_type::string;
	} else if (std::holds_alternative<array_t>(m_value)) {
		return json_type::array;
	} else if (std::holds_alternative<object_t>(m_value)) {
		return json_type::object;
	} else {
		return json_type::null;
	}
}

json::result_t json::read(std::string_view text) {
	result_t ret;
	auto result = detail::parser_t(text).parse();
	if (!result.unexpected.empty()) {
		for (detail::unexpect_t const& err : result.unexpected) {
			std::stringstream str;
			str << "line " << err.loc.line << " col " << err.loc.col << ": ";
			str << err.text;
			if (!err.expected.empty()) {
				str << " (expected ";
				bool first = true;
				for (auto tk : err.expected) {
					if (!first) { str << " or "; }
					first = false;
					str << detail::tk_type_str[(std::size_t)tk];
				}
				str << ")";
				ret.errors.push_back({str.str(), error_type::parse});
			} else {
				ret.errors.push_back({str.str(), error_type::syntax});
			}
		}
	}
	if (result.json_) {
		m_value = std::move(result.json_->m_value);
		ret.failure = false;
	}
	return ret;
}

json::result_t json::load(std::string const& path) {
	if (auto file = std::ifstream(path)) {
		std::stringstream str;
		str << file.rdbuf();
		return read(str.str());
	}
	std::string err = path;
	err += " not found";
	return {{{std::move(err), error_type::io}}, true};
}

std::ostream& json::serialize(std::ostream& out, serial_opts_t const& opts) const { return serialize_json(*this, opts, out, 0, true); }

std::string json::to_string(serial_opts_t const& opts) const {
	std::stringstream str;
	serialize(str, opts);
	return str.str();
}

bool json::save(std::string const& path, serial_opts_t const& opts) const {
	if (auto file = std::ofstream(path); file && serialize(file, opts)) { return true; }
	return false;
}

bool json::contains(std::string const& key) const noexcept {
	if (auto obj = detail::to_t<object_t>(m_value)) { return obj->value.find(key) != obj->value.end(); }
	return false;
}

bool json::contains(std::size_t index) const noexcept {
	if (auto obj = detail::to_t<array_t>(m_value)) { return index < obj->value.size(); }
	return false;
}

json* json::find(std::string const& str) const noexcept {
	if (auto obj = detail::to_t<object_t>(m_value)) {
		if (auto it = obj->value.find(str); it != obj->value.end()) { return &*it->second; }
	}
	return nullptr;
}

json* json::find(std::size_t idx) const noexcept {
	if (auto obj = detail::to_t<array_t>(m_value)) {
		if (idx < obj->value.size()) { return &*obj->value[idx]; }
	}
	return nullptr;
}

json const& json::get(std::string const& key) const noexcept {
	if (auto ret = find(key)) { return *ret; }
	static json const fallback;
	return fallback;
}

json& json::operator[](std::string const& str) const {
	if (auto ret = find(str)) { return *ret; }
	throw not_found_exception{};
}

json& json::operator[](std::size_t idx) const {
	if (auto ret = find(idx)) { return *ret; }
	throw not_found_exception{};
}

json& json::set(json_type type, std::string value) {
	switch (type) {
	case json_type::null: m_value = null_t{}; break;
	case json_type::boolean: m_value = boolean_t{value == "true"}; break;
	case json_type::number: m_value = number_t{std::move(value)}; break;
	default: m_value = string_t{std::move(value)}; break;
	}
	return *this;
}

json& json::set(json value) {
	m_value = std::move(value.m_value);
	return *this;
}

json& json::push_back(json value) {
	if (type() != json_type::array) { m_value = array_t{}; }
	auto& array = std::get<array_t>(m_value);
	array.value.push_back(detail::make<json>(std::move(value.m_value)));
	return *array.value.back();
}

json& json::insert(std::string const& key, json value) {
	if (type() != json_type::object) { m_value = object_t{}; }
	auto& object = std::get<object_t>(m_value);
	auto [it, _] = object.value.emplace(key, detail::make<json>(std::move(value.m_value)));
	return *it->second;
}

std::ostream& operator<<(std::ostream& out, json const& json) { return json.serialize(out); }

std::ostream& operator<<(std::ostream& out, json::result_t const& result) {
	out << (result.failure ? "-- Failure" : (result.errors.empty() ? "== Success" : "++ Partial")) << '\n';
	for (auto const& error : result.errors) { out << "  " << json::error_type_names[(std::size_t)error.type] << " error: " << error.message << '\n'; }
	return out;
}
} // namespace dj
