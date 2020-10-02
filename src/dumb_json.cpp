#include <algorithm>
#include <cstring>
#include <map>
#include <dumb_json/dumb_json.hpp>
#include <str_format/str_format.hpp>

namespace dj {
namespace {
template <typename... Args>
void log_err(std::string_view fmt, Args&&... args) {
	if (g_log_error) {
		g_log_error(kt::format_str(fmt, std::forward<Args>(args)...));
	}
}

constexpr std::size_t g_bufSize = 128;
constexpr char const* g_name = "dumbjson";

struct ignore_pair final {
	struct sequence final {
		std::pair<char, char> match;
		std::int64_t count = 0;
	};
	std::vector<sequence> sequences;

	std::int64_t process(char c);
	std::int64_t count(std::pair<char, char> pair) const;
	void add(std::pair<char, char> match);
};

std::int64_t ignore_pair::process(char c) {
	std::int64_t total = 0;
	for (auto& sequence : sequences) {
		if (c == sequence.match.first && sequence.match.first == sequence.match.second) {
			sequence.count = (sequence.count == 0) ? 1 : 0;
		} else {
			if (c == sequence.match.second) {
				--sequence.count;
			} else if (c == sequence.match.first) {
				++sequence.count;
			}
		}
		total += sequence.count;
	}
	return total;
}

std::int64_t ignore_pair::count(std::pair<char, char> pair) const {
	auto search = std::find_if(sequences.begin(), sequences.end(),
							   [pair](auto const& seq) -> bool { return seq.match.first == pair.first && seq.match.second == pair.second; });
	return search != sequences.end() ? search->count : 0;
}

void ignore_pair::add(std::pair<char, char> match) {
	sequences.push_back({match, 0});
}

bool is_escaped(std::string_view text, std::size_t this_char, char escape = '\\') {
	bool b_escaping = false;
	for (; this_char > 0 && text.at(this_char - 1) == escape; --this_char) {
		b_escaping = !b_escaping;
	}
	return b_escaping;
}

bool is_whitespace(char c, std::uint64_t* out_line = nullptr) {
	if (c == '\n' || c == '\r') {
		if (out_line) {
			++*out_line;
		}
		return true;
	}
	return std::isspace(c);
}

bool is_bool(std::string_view text) {
	static std::array<std::string_view, 2> const s_valid = {"true", "false"};
	if (!text.empty()) {
		for (auto const& valid : s_valid) {
			if (valid.size() <= text.size() && std::string_view(text.data(), valid.size()) == valid) {
				return true;
			}
		}
	}
	return false;
}

void trim(std::string_view& out_text, std::size_t& out_line) {
	while (!out_text.empty() && is_whitespace(out_text.at(0), &out_line)) {
		out_text = out_text.substr(1);
	}
}

void count_newlines(std::string_view text, std::uint64_t& out_line) {
	while (!text.empty()) {
		is_whitespace(text.at(0), &out_line);
		text = text.substr(1);
	}
}

std::string sanitise(std::string_view text, std::size_t begin = 0, std::size_t end = 0) {
	std::string ret;
	ret.reserve(end - begin);
	if (end == 0) {
		end = text.size();
	}
	for (std::size_t idx = begin; idx < end; ++idx) {
		if (!is_escaped(text, idx + 1)) {
			ret += text.at(idx);
		}
	}
	return ret;
}

data_type parse_type(std::string_view text) {
	std::uint64_t line = 0;
	trim(text, line);
	if (!text.empty()) {
		char const c = text.at(0);
		switch (c) {
		case '[':
			return data_type::array;
		case '{':
			return data_type::object;
		case '\"':
			return data_type::string;
		default: {
			if (is_bool(text)) {
				return data_type::boolean;
			} else if (text.find(".") == std::string::npos) {
				return data_type::integer;
			} else {
				return data_type::floating;
			}
		}
		}
	}
	return data_type::none;
}

std::string_view parse_primitive(std::string_view& out_text, std::uint64_t& out_line) {
	if (!out_text.empty() && out_text.at(0) != '\"' && out_text.at(0) != '[' && out_text.at(0) != '{') {
		auto text = out_text;
		trim(text, out_line);
		if (text.empty()) {
			log_err("[{}] Unexpected end of string (line: {})", g_name, out_line);
			return {};
		}
		static constexpr std::array end_chars = {',', '}', ']'};
		auto is_end_char = [](char c) -> bool { return std::isspace(c) || std::find(end_chars.begin(), end_chars.end(), c) != end_chars.end(); };
		for (std::size_t end = 0; end < text.size(); ++end) {
			if (is_end_char(text.at(end))) {
				text = out_text;
				count_newlines(out_text.substr(0, end), out_line);
				out_text = out_text.substr(end + 1);
				trim(out_text, out_line);
				return text.substr(0, end);
			}
		}
		text = out_text;
		out_text = {};
		return text;
	}
	out_text = {};
	return {};
}

std::string_view parse_string(std::string_view& out_text, std::uint64_t& out_line) {
	if (!out_text.empty() && out_text.at(0) == '\"') {
		auto text = out_text;
		if (text.size() < 2) {
			log_err("[{}] Unexpected end of string (line: {})", g_name, out_line);
			out_text = {};
			return {};
		}
		for (std::size_t end = 1; end < text.size(); ++end) {
			bool b_escaping = is_escaped(text, end);
			if (!b_escaping && text.at(end) == '\"') {
				count_newlines(out_text.substr(0, end), out_line);
				out_text = out_text.substr(end + 1);
				trim(out_text, out_line);
				return text.substr(1, end - 1);
			}
		}
	}
	out_text = {};
	return {};
}

std::string_view parse_object_or_array(std::string_view& out_text, std::uint64_t& out_line, bool b_array) {
	char const start_char = b_array ? '[' : '{';
	if (!out_text.empty() && out_text.at(0) == start_char) {
		char const end_char = b_array ? ']' : '}';
		ignore_pair end_escapes;
		end_escapes.add({'{', '}'});
		end_escapes.add({'"', '"'});
		end_escapes.add({'[', ']'});
		auto text = out_text;
		for (std::size_t end = 0; end < text.size(); ++end) {
			char c = text.at(end);
			bool b_escaping = end_escapes.count({'\"', '\"'}) > 0 ? is_escaped(text, end) : false;
			if (b_escaping) {
				continue;
			}
			auto escape_stack = end_escapes.process(c);
			if (escape_stack <= 0 && text.at(end) == end_char) {
				count_newlines(out_text.substr(0, end - 1), out_line);
				out_text = out_text.substr(end);
				trim(out_text, out_line);
				return text.substr(0, end + 1);
			}
		}
	}
	out_text = {};
	return {};
}

std::string parse_key(std::string_view& out_text, std::uint64_t& out_line) {
	static constexpr std::string_view s_failure = "failed to extract key!";
	if (out_text.empty()) {
		log_err("[{}] Unexpected end of string (line: {}), {}", g_name, out_line, s_failure);
		return {};
	}
	trim(out_text, out_line);
	char c = out_text.at(0);
	if (c != '\"') {
		log_err("[{}] Expected: '\"' (line: {}), {}!", g_name, out_line, s_failure);
		return {};
	}
	std::string ret = sanitise(parse_string(out_text, out_line));
	if (out_text.empty() || out_text.at(0) != ':') {
		log_err("[{}] Expected ':' after key [{}] (line: {}), {}", g_name, ret, out_line, s_failure);
		return {};
	}
	out_text = out_text.substr(1);
	trim(out_text, out_line);
	return ret;
}

std::pair<std::string_view, data_type> parse_value(std::string_view& out_text, std::uint64_t& out_line, std::uint64_t& out_value_start_line) {
	static constexpr std::string_view s_failure = "failed to extract value!";
	trim(out_text, out_line);
	if (out_text.empty()) {
		log_err("[{}] Unexpected end of string (line: {}), {}", g_name, out_line, s_failure);
		return {};
	}
	out_value_start_line = out_line;
	data_type type = parse_type(out_text);
	std::string_view value;
	switch (type) {
	case data_type::boolean:
	case data_type::integer:
	case data_type::floating: {
		value = parse_primitive(out_text, out_line);
		break;
	}
	case data_type::string: {
		value = parse_string(out_text, out_line);
		break;
	}
	case data_type::object: {
		value = parse_object_or_array(out_text, out_line, false);
		break;
	}
	case data_type::array: {
		value = parse_object_or_array(out_text, out_line, true);
		break;
	}
	case data_type::none: {
		log_err("[{}] Unsupported/unrecognised data type! (line: {})", g_name, out_line);
		break;
	}
	}
	if (value.empty()) {
		out_text = {};
		return {};
	}
	trim(out_text, out_line);
	if (!out_text.empty() && out_text.at(0) == ',') {
		out_text = out_text.substr(1);
		trim(out_text, out_line);
	}
	return {value, type};
}

template <typename T>
T to_numeric(std::string_view text) {
	T ret = {};
	std::array<char, g_bufSize> buffer;
	std::memcpy(buffer.data(), text.data(), std::min(text.size(), buffer.size()));
	buffer.at(text.size()) = '\0';
	try {
		if constexpr (std::is_integral_v<T>) {
			ret = (T)std::atoi(buffer.data());
		} else {
			ret = (T)std::atof(buffer.data());
		}
	} catch (const std::exception& e) {
		log_err("[{}] Failed to parse [{}] into numeric type! {}", g_name, text, e.what());
	}
	return ret;
}

base_ptr create(data_type type, std::string_view value, std::uint64_t& out_line, std::int8_t max_depth) {
	switch (type) {
	case data_type::boolean: {
		auto new_bool = std::make_unique<boolean>();
		new_bool->value = value == "true" ? true : false;
		return new_bool;
	}
	case data_type::integer: {
		auto new_integer = std::make_unique<integer>();
		new_integer->value = to_numeric<std::int64_t>(value);
		return new_integer;
	}
	case data_type::floating: {
		auto new_float = std::make_unique<floating>();
		new_float->value = to_numeric<double>(value);
		return new_float;
	}
	case data_type::string: {
		auto new_string = std::make_unique<string>();
		new_string->value = sanitise(value);
		return new_string;
	}
	case data_type::object: {
		auto new_object = std::make_unique<object>();
		if (new_object->read(value, max_depth - 1, &out_line)) {
			return new_object;
		}
		return {};
	}
	case data_type::array: {
		auto new_array = std::make_unique<array>();
		if (new_array->read(value, &out_line)) {
			return new_array;
		}
		return {};
	}
	case data_type::none: {
		return {};
	}
	}
	return {};
}

void advance(data_type type, std::string_view& start, uint64_t& out_line, char final) {
	if (type == data_type::array || type == data_type::object) {
		char const end_char = type == data_type::object ? '}' : ']';
		trim(start, out_line);
		if (!start.empty() && start.at(0) == end_char) {
			start = start.substr(1);
			trim(start, out_line);
		}
	}
	if (!start.empty()) {
		if (start.at(0) == final) {
			start = {};
		} else if (start.at(0) == ',') {
			start = start.substr(1);
			trim(start, out_line);
		}
	}
}

struct indenter final {
	serial_opts const* opts = nullptr;
	std::uint8_t indent = 0;
	bool b_indent = true;
};
struct newliner final {
	serial_opts const* opts = nullptr;
};
struct spacer final {
	serial_opts const* opts = nullptr;
	std::uint8_t spaces = 1;
};
std::ostream& operator<<(std::ostream& out, indenter s) {
	while (s.b_indent && s.indent-- > 0) {
		if (s.opts->space_indent > 0) {
			for (auto i = s.opts->space_indent; i > 0; --i) {
				out << ' ';
			}
		} else {
			out << "\t";
		}
	}
	return out;
}
std::ostream& operator<<(std::ostream& out, newliner n) {
	if (n.opts->pretty) {
		out << n.opts->newline;
	}
	return out;
}
std::ostream& operator<<(std::ostream& out, spacer s) {
	while (s.opts->pretty && s.spaces-- > 0) {
		out << ' ';
	}
	return out;
}

std::string serialise_str(std::string_view str) {
	std::string ret;
	ret.reserve(str.size());
	for (auto c : str) {
		if (c == '"' || c == '\\') {
			ret += '\\';
		}
		ret += c;
	}
	return ret;
}

std::string serialise_value(base const& base) {
	switch (base.type()) {
	case data_type::boolean: {
		if (auto p_bool = base.cast<boolean const>()) {
			return p_bool->value ? "true" : "false";
		}
		return "n/a";
	}
	case data_type::integer: {
		if (auto p_int = base.cast<integer const>()) {
			return std::to_string(p_int->value);
		}
		return "n/a";
	}
	case data_type::floating: {
		if (auto p_float = base.cast<floating const>()) {
			return std::to_string(p_float->value);
		}
		return "n/a";
	}
	case data_type::string: {
		if (auto p_string = base.cast<string const>()) {
			auto val = serialise_str(p_string->value);
			std::string ret = "\"";
			ret += std::move(val);
			ret += "\"";
			return ret;
		}
		return "n/a";
	}
	default: {
		break;
	}
	}
	return {};
}

std::stringstream& serialise_entry(serial_opts const& opts, std::stringstream& out, std::uint8_t indent, base const& entry, bool b_last) {
	auto const type = entry.type();
	if (detail::is_type_value_type(type)) {
		out << serialise_value(entry);
	} else if (type == data_type::object) {
		if (auto p_object = entry.cast<object const>()) {
			p_object->serialise(out, opts, indent + 1);
		}
	} else if (type == data_type::array) {
		if (auto p_array = entry.cast<array const>()) {
			p_array->serialise(out, opts, indent + 1);
		}
	}
	if (!b_last) {
		out << ',';
	}
	return out;
}

base_ptr move_entry(base&& base) {
	base_ptr ret;
	switch (base.type()) {
	case data_type::boolean: {
		ret = std::make_unique<boolean>(std::move(*base.cast<boolean>()));
		break;
	}
	case data_type::integer: {
		ret = std::make_unique<integer>(std::move(*base.cast<integer>()));
		break;
	}
	case data_type::floating: {
		ret = std::make_unique<floating>(std::move(*base.cast<floating>()));
		break;
	}
	case data_type::string: {
		ret = std::make_unique<string>(std::move(*base.cast<string>()));
		break;
	}
	case data_type::object: {
		ret = std::make_unique<object>(std::move(*base.cast<object>()));
		break;
	}
	case data_type::array: {
		ret = std::make_unique<array>(std::move(*base.cast<array>()));
		break;
	}
	default:
		break;
	}
	return ret;
}
} // namespace

bool object::read(std::string_view text, std::int8_t max_depth, std::uint64_t* line) {
	if (max_depth < 0) {
		log_err("[{}] Max depth exceeded, aborting!", g_name);
		return false;
	}
	std::uint64_t line_ = 1;
	if (!line) {
		line = &line_;
	}
	trim(text, *line);
	auto start = text;
	if (!start.empty() && start.at(0) == '{') {
		start = start.substr(1);
		trim(start, *line);
		while (!start.empty()) {
			auto key = parse_key(start, *line);
			auto value_line = *line;
			auto [value, type] = parse_value(start, *line, value_line);
			if (auto new_entry = create(type, value, value_line, max_depth)) {
				if (!key.empty()) {
					fields[std::string(key)] = std::move(new_entry);
				} else {
					log_err("[{}] Empty key! Skipping value (line: {})", g_name, *line);
				}
			}
			advance(type, start, *line, '}');
		}
		return true;
	}
	return false;
}

base* object::add(std::string_view key, std::string_view value, data_type type, std::int8_t max_depth) {
	std::string id = key.data();
	if (id.empty()) {
		log_err("[{}] Empty key!", g_name);
		return nullptr;
	}
	if (fields.find(id) != fields.end()) {
		log_err("[{}] Duplicate key [{}]!", g_name, id);
		return nullptr;
	}
	std::uint64_t line = 1, value_line = line;
	if (type == data_type::none) {
		auto pair = parse_value(value, line, value_line);
		value = pair.first;
		type = pair.second;
	}
	if (auto new_entry = create(type, value, value_line, max_depth)) {
		auto ret = new_entry.get();
		fields.emplace(std::move(id), std::move(new_entry));
		return ret;
	}
	log_err("[{}] Error parsing value!", g_name);
	return nullptr;
}

bool object::add(std::string_view key, base&& entry) {
	std::string id = key.data();
	if (id.empty()) {
		log_err("[{}] Empty key!", g_name);
		return false;
	}
	if (entry.type() == data_type::none) {
		log_err("[{}] Invalid type!", g_name);
		return false;
	}
	if (fields.find(id) != fields.end()) {
		log_err("[{}] Duplicate key [{}]!", g_name, id);
		return false;
	}
	fields.emplace(std::move(id), move_entry(std::move(entry)));
	return true;
}

bool object::contains(std::string const& id) const {
	return fields.find(id) != fields.end();
}

std::stringstream& object::serialise(std::stringstream& out, serial_opts const& options, std::uint8_t indent) const {
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
			std::map<std::string, base const*> map;
			for (auto const& [key, entry] : fields) {
				map[serialise_str(key)] = entry.get();
			}
			for (auto const& [key, p_entry] : map) {
				if (index > 0) {
					out << n;
				}
				out << t << '\"' << key << "\":" << s;
				serialise_entry(options, out, indent, *p_entry, (index == fields.size() - 1));
				++index;
			}
		} else {
			for (auto const& [key, entry] : fields) {
				if (index > 0) {
					out << n;
				}
				out << t << '\"' << serialise_str(key) << "\":" << s;
				serialise_entry(options, out, indent, *entry, (index == fields.size() - 1));
				++index;
			}
		}
		--t.indent;
		out << n << t << '}';
	}
	return out;
}

std::string object::serialise(serial_opts const& options, std::uint8_t indent) const {
	std::stringstream str;
	serialise(str, options, indent);
	return str.str();
}

bool array::read(std::string_view text, std::uint64_t* line) {
	std::uint64_t line_ = 1;
	if (!line) {
		line = &line_;
	}
	trim(text, *line);
	auto start = text;
	if (!start.empty() && start.at(0) == '[') {
		start = start.substr(1);
		while (!start.empty()) {
			auto value_line = *line;
			auto [value, type] = parse_value(start, *line, value_line);
			advance(type, start, *line, ']');
			if (held_type == data_type::none) {
				held_type = type;
			}
			if (auto new_entry = create(type, value, value_line, 8)) {
				fields.push_back(std::move(new_entry));
			}
			trim(start, *line);
			if (!start.empty() && start.at(0) == ']') {
				break;
			}
		}
	}
	return held_type != data_type::none;
}

base* array::add(std::string_view value, data_type type, std::int8_t max_depth) {
	if (value.empty()) {
		log_err("[{}] Empty value!", g_name);
		return nullptr;
	}
	if (!fields.empty() && type != data_type::none && type != held_type) {
		log_err("[{}] Invalid type!", g_name);
		return nullptr;
	}
	std::uint64_t line = 1, value_line = line;
	if (type == data_type::none) {
		auto pair = parse_value(value, line, value_line);
		value = pair.first;
		type = pair.second;
	}
	if (auto new_entry = create(type, value, value_line, max_depth)) {
		auto ret = new_entry.get();
		fields.push_back(std::move(new_entry));
		return ret;
	}
	return nullptr;
}

bool array::add(base&& entry) {
	if (entry.type() != data_type::none && (held_type == data_type::none || entry.type() == held_type)) {
		fields.push_back(move_entry(std::move(entry)));
		held_type = entry.type();
		return true;
	}
	log_err("[{}] Invalid type!", g_name);
	return false;
}

std::stringstream& array::serialise(std::stringstream& out, serial_opts const& options, std::uint8_t indent) const {
	if (fields.empty()) {
		out << "[]";
	} else {
		indenter t{&options, indent};
		newliner n{&options};
		out << '[' << n;
		++t.indent;
		std::size_t index = 0;
		for (auto const& entry : fields) {
			if (index > 0) {
				out << n;
			}
			out << t;
			serialise_entry(options, out, indent, *entry, (index == fields.size() - 1));
			++index;
		}
		--t.indent;
		out << n << t << ']';
	}
	return out;
}
} // namespace dj

dj::base_ptr dj::deserialise(std::string_view text, std::uint8_t max_depth) {
	std::uint64_t line = 1;
	auto [value, type] = parse_value(text, line, line);
	return create(type, value, line, max_depth);
}
