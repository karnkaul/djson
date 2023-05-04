#include <compat/api.hpp>
#include <detail/scanner.hpp>
#include <djson/json.hpp>
#include <cassert>
#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dj {
namespace detail {
bool str_to_d(std::string_view str, std::int64_t& out) {
	auto const [_, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
	return ec == std::errc{};
}

bool str_to_d(std::string_view str, std::uint64_t& out) {
	auto const [_, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
	return ec == std::errc{};
}

class HeapString {
  public:
	HeapString() = default;
	HeapString(std::string_view const str) : m_ptr{detail::make_char_buf(str.size())}, m_size{str.size()} { std::memcpy(m_ptr.get(), str.data(), str.size()); }

	std::string_view view() const { return m_ptr ? std::string_view{m_ptr.get(), m_size} : std::string_view{""}; }
	operator std::string_view() const { return view(); }

  private:
	std::unique_ptr<char[]> m_ptr{};
	std::size_t m_size{};
};
} // namespace detail

namespace {
struct Printer : ParseError::Handler {
	void operator()(ParseError const& error) override {
		std::cerr << "Error: " << error.message << "  '" << error.token << "' | line " << error.line << ":" << error.column << " \n";
	}

	static Printer& instance() {
		static auto ret = Printer{};
		return ret;
	}
};

template <typename... T>
struct Visitor : T... {
	using T::operator()...;
};

template <typename... T>
Visitor(T...) -> Visitor<T...>;

struct Error {};

struct FifoMap {
	static constexpr auto npos = std::string_view::npos;

	std::unordered_map<std::string_view, std::size_t> map{};
	std::vector<detail::HeapString> keys{};
	std::vector<Json> values{};

	std::size_t insert_or_assign(std::string_view key, Json&& value) {
		auto index = keys.size();
		if (auto const it = map.find(key); it != map.end()) {
			index = it->second;
			assert(index < values.size());
			values[index] = std::move(value);
		} else {
			keys.push_back(key);
			values.push_back(std::move(value));
			map.insert_or_assign(keys.back(), index);
		}
		return index;
	}

	std::size_t find(std::string_view const key) const {
		if (auto const it = map.find(key); it != map.end()) {
			assert(it->second < keys.size() && it->second < values.size());
			return it->second;
		}
		return npos;
	}

	bool contains(std::string_view const key) const { return find(key) != npos; }
};

std::string unescape(std::string_view in) {
	auto ret = std::string{};
	ret.reserve(in.size());
	while (!in.empty()) {
		char c = in[0];
		if (c == '\\' && in.size() > 1) {
			in = in.substr(1);
			c = in[0];
			switch (c) {
			case 't': ret += '\t'; break;
			case 'n': ret += '\n'; break;
			case 'r': ret += '\r'; break;
			case '\\': ret += '\\'; break;
			case '\"': ret += '\"'; break;
			case 'b': ret.pop_back(); break;
			default: break;
			}
			in = in.substr(1);
			continue;
		}
		ret += c;
		in = in.substr(1);
	}
	return ret;
}

std::string escape(std::string_view in) {
	auto ret = std::string{};
	ret.reserve(in.size());
	for (char const c : in) {
		switch (c) {
		case '\n': ret += R"(\n)"; break;
		case '\t': ret += R"(\t)"; break;
		case '\"': ret += R"(\")"; break;
		case '\\': ret += R"(\\)"; break;
		default: ret += c; break;
		}
	}
	return ret;
}

static auto const null_v = Json{nullptr};
} // namespace

struct Json::Impl {
	using Null = std::nullptr_t;
	using String = std::string;
	using Object = FifoMap;
	using Array = std::vector<Json>;
	struct Number {
		std::string serialized{};
	};
	using Payload = std::variant<Null, Boolean, Number, String, Object, Array>;
	struct Parser;

	static Json make(Payload payload) {
		auto ret = Json{Construct{}};
		ret.m_impl->payload = std::move(payload);
		return ret;
	}

	static Payload clone_object(Object const& object) {
		auto ret = Object{};
		for (std::size_t index = 0; index < object.keys.size(); ++index) {
			assert(index < object.values.size());
			auto const& json = object.values[index];
			ret.insert_or_assign(object.keys[index], make(clone(json.m_impl->payload)));
		}
		return ret;
	}

	static Payload clone_array(Array const& array) {
		auto ret = Array{};
		for (auto const& json : array) { ret.push_back(make(clone(json.m_impl->payload))); }
		return ret;
	}

	static Payload clone(Payload const& rhs) {
		auto const visitor = Visitor{
			[](auto const& t) { return Payload{t}; },
			[](Object const& o) { return clone_object(o); },
			[](Array const& a) { return clone_array(a); },
		};
		return std::visit(visitor, rhs);
	}

	static std::unique_ptr<Impl> clone(std::unique_ptr<Impl> const& rhs) {
		if (!rhs) { return {}; }
		auto ret = std::make_unique<Impl>();
		ret->payload = clone(rhs->payload);
		return ret;
	}

	ObjectProxy& refresh_object() {
		object.m_impl = this;
		return object;
	}

	ArrayProxy& refresh_array() {
		array.m_impl = this;
		return array;
	}

	Payload payload{};
	ObjectProxy object{};
	ArrayProxy array{};
};

struct Whitespace {
	std::string_view token{};
	std::size_t spacing{};
	friend std::ostream& operator<<(std::ostream& out, Whitespace const& ws) {
		if (ws.spacing == 0) { return out; }
		return out << ws.token;
	}
};

struct Indent {
	std::size_t spacing{};
	std::size_t indent{};
	friend std::ostream& operator<<(std::ostream& out, Indent const& indent) {
		auto const s = indent.spacing * indent.indent;
		for (std::size_t i = 0; i < s; ++i) { out << Whitespace{" ", indent.spacing}; }
		return out;
	}
};

struct Json::Serializer {
	std::ostream& out;
	std::size_t spacing{2};

	std::size_t indent{};

	std::ostream& operator()(Json const& json) const {
		if (json.is_null()) { return out << "null"; }
		if (json.is_bool()) { return out << (json.as_bool() ? "true" : "false"); }
		if (json.is_number()) { return out << std::get<Impl::Number>(json.m_impl->payload).serialized; }
		if (json.is_string()) { return quote(escape(json.as_string())); }
		if (json.is_array()) { return array(json); }
		if (json.is_object()) { return object(json); }
		return out;
	}

	std::ostream& quote(std::string_view const str) const { return out << '"' << str << '"'; }

	std::ostream& array(Json const& json) const {
		auto s = Serializer{out, spacing, indent + 1};
		out << "[" << Whitespace{"\n", spacing};
		bool first{true};
		for (auto const& j : json.array_view()) {
			if (!first) { out << "," << Whitespace{"\n", spacing}; }
			first = false;
			out << Indent{s.spacing, s.indent};
			s(j);
		}
		out << Whitespace{"\n", spacing} << Indent{spacing, indent} << "]";
		return out;
	}

	std::ostream& object(Json const& json) const {
		auto s = Serializer{out, spacing, indent + 1};
		out << "{" << Whitespace{"\n", spacing};
		bool first{true};
		for (auto const& [k, j] : json.object_view()) {
			if (!first) { out << "," << Whitespace{"\n", spacing}; }
			first = false;
			out << Indent{s.spacing, s.indent};
			quote(k);
			out << ":" << Whitespace{" ", spacing};
			s(j);
		}
		out << Whitespace{"\n", spacing} << Indent{spacing, indent} << "}";
		return out;
	}
};

struct Json::Impl::Parser {
	using Type = detail::Token::Type;

	ParseError::Handler& handler;
	detail::Scanner& scanner;

	detail::Token previous{};
	detail::Token current{};
	bool error{};

	Json parse() {
		advance();
		auto ret = Json{Construct{}};
		try {
			ret.m_impl->payload = make_payload();
		} catch (Error) {}
		return ret;
	}

	Payload make_payload() {
		if (match(Type::eNull)) { return nullptr; }
		if (match(Type::eTrue)) { return true_v; }
		if (match(Type::eFalse)) { return false_v; }
		if (match(Type::eNumber)) { return Number{std::string{lexeme()}}; }
		if (match(Type::eString)) { return string(); }
		if (match(Type::eBraceL)) { return object(); }
		if (match(Type::eSqBrL)) { return array(); }
		auto const column = static_cast<std::uint32_t>(current.loc.span.first - current.loc.line_start) + 1;
		handler(ParseError{"Invalid token", current.lexeme(scanner.text), current.loc.line, column});
		throw Error{};
	}

	std::string_view lexeme() const { return previous.lexeme(scanner.text); }

	std::string string() const { return unescape(lexeme()); }

	Array array() {
		auto ret = Array{};
		if (peek().type != Type::eSqBrR) {
			do { ret.push_back(make(make_payload())); } while (match(Type::eComma));
		}
		consume(Type::eSqBrR);
		return ret;
	}

	Object object() {
		auto ret = Object{};
		if (peek().type != Type::eBraceR) {
			do {
				consume(Type::eString);
				auto key = string();
				consume(Type::eColon);
				ret.insert_or_assign(std::move(key), make(make_payload()));
			} while (match(Type::eComma));
		}
		consume(Type::eBraceR);
		return ret;
	}

	detail::Token const& advance() {
		previous = current;
		return current = scanner.scan();
	}

	bool match(Type expected) {
		if (peek().type == expected) {
			advance();
			return true;
		}
		return false;
	}

	void consume(Type expected) noexcept(false) {
		if (!match(expected)) {
			auto const column = static_cast<std::uint32_t>(current.loc.span.first - current.loc.line_start) + 1;
			handler(ParseError{"Invalid token", detail::Token::type_string(current.type), current.loc.line, column});
			throw Error{};
		}
	}

	detail::Token const& peek() const { return current; }
	bool at_end() const { return current.type == Type::eEof; }
};

Json::Json() noexcept = default;
Json::Json(Construct) : m_impl{std::make_unique<Impl>()} {}
Json::Json(Json&& rhs) noexcept : Json() { swap(rhs); }
Json& Json::operator=(Json rhs) noexcept { return (swap(rhs), *this); }
Json::~Json() noexcept = default;
Json::Json(Json const& rhs) : m_impl{Impl::clone(rhs.m_impl)} {}

void Json::swap(Json& rhs) noexcept { std::swap(m_impl, rhs.m_impl); }

Json Json::parse(std::string_view const text, ParseError::Handler* handler) {
	if (!handler) { handler = &Printer::instance(); }
	auto scanner = detail::Scanner{text};
	auto parser = Impl::Parser{*handler, scanner};
	return parser.parse();
}

Json Json::from_file(char const* path, ParseError::Handler* handler) {
	if (!handler) { handler = &Printer::instance(); }
	auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
	if (!file) {
		auto msg = std::string{"File ["};
		msg += path;
		msg += "] not found";
		(*handler)(ParseError{msg});
		return null_v;
	}
	auto const size = file.tellg();
	file.seekg({});
	auto bytes = std::string(static_cast<std::size_t>(size), '\0');
	file.read(bytes.data(), size);
	return parse(bytes, handler);
}

Json::Json(AsNumber, std::string number) : Json(Construct{}) { m_impl->payload = Impl::Number{std::move(number)}; }
Json::Json(AsString, std::string_view const string) : Json(Construct{}) { m_impl->payload = unescape(string); }

Json::Json(std::nullptr_t) : Json(Construct{}) { m_impl->payload = Impl::Null{}; }
Json::Json(Boolean const boolean) : Json(Construct{}) { m_impl->payload = boolean; }

bool Json::is_null() const { return !m_impl || std::holds_alternative<Impl::Null>(m_impl->payload); }
bool Json::is_bool() const { return m_impl && std::holds_alternative<Boolean>(m_impl->payload); }
bool Json::is_number() const { return m_impl && std::holds_alternative<Impl::Number>(m_impl->payload); }
bool Json::is_string() const { return m_impl && std::holds_alternative<Impl::String>(m_impl->payload); }
bool Json::is_object() const { return m_impl && std::holds_alternative<Impl::Object>(m_impl->payload); }
bool Json::is_array() const { return m_impl && std::holds_alternative<Impl::Array>(m_impl->payload); }

Boolean Json::as_bool(Boolean const fallback) const {
	if (!m_impl) { return fallback; }
	if (auto const* ret = std::get_if<Boolean>(&m_impl->payload)) { return *ret; }
	return fallback;
}

std::int64_t Json::as_i64(std::int64_t const fallback) const { return from_number<std::int64_t>(fallback); }
std::uint64_t Json::as_u64(std::uint64_t const fallback) const { return from_number<std::uint64_t>(fallback); }
double Json::as_double(double const fallback) const { return from_number<double>(fallback); }

std::string_view Json::as_string(std::string_view const fallback) const {
	if (!m_impl) { return fallback; }
	if (auto const* ret = std::get_if<std::string>(&m_impl->payload)) { return *ret; }
	return fallback;
}

Json& Json::push_back(Json value) {
	if (!m_impl) { m_impl = std::make_unique<Impl>(); }
	auto* array = std::get_if<Impl::Array>(&m_impl->payload);
	if (!array) {
		m_impl->payload = Impl::Array{};
		array = &std::get<Impl::Array>(m_impl->payload);
	}
	if (value.m_impl) {
		array->push_back(Impl::make(std::move(value.m_impl->payload)));
	} else {
		array->emplace_back();
	}
	return array->back();
}

Json& Json::operator[](std::size_t const index) {
	if (!m_impl) { m_impl = std::make_unique<Impl>(); }
	auto* array = std::get_if<Impl::Array>(&m_impl->payload);
	if (!array) {
		m_impl->payload = Impl::Array{};
		array = &std::get<Impl::Array>(m_impl->payload);
	}
	if (array->size() <= index) { array->resize(index + 1); }
	return (*array)[index];
}

Json const& Json::operator[](std::size_t const index) const {
	if (!m_impl) { return null_v; }
	auto* array = std::get_if<Impl::Array>(&m_impl->payload);
	if (!array || array->size() <= index) { return null_v; }
	return (*array)[index];
}

bool Json::contains(std::string_view const key) const {
	if (!m_impl) { return false; }
	auto const* object = std::get_if<Impl::Object>(&m_impl->payload);
	if (!object) { return false; }
	return object->contains(key);
}

Json& Json::assign(std::string_view const key, Json value) {
	auto& ret = (*this)[key];
	ret.m_impl = std::move(value.m_impl);
	return ret;
}

Json& Json::operator[](std::string_view const key) {
	if (!m_impl) { m_impl = std::make_unique<Impl>(); }
	auto* object = std::get_if<Impl::Object>(&m_impl->payload);
	if (!object) {
		m_impl->payload = Impl::Object{};
		object = &std::get<Impl::Object>(m_impl->payload);
	}
	auto index = object->find(key);
	if (index == FifoMap::npos) { index = object->insert_or_assign(std::string{key}, Impl::make({})); }
	return object->values[index];
}

Json const& Json::operator[](std::string_view const key) const {
	if (!m_impl) { return null_v; }
	auto* object = std::get_if<Impl::Object>(&m_impl->payload);
	if (!object) { return null_v; }
	auto index = object->find(key);
	if (index == FifoMap::npos) { return null_v; }
	return object->values[index];
}

auto Json::array_view() -> ArrayProxy& { return m_impl->refresh_array(); }
auto Json::array_view() const -> ArrayProxy const& { return m_impl->refresh_array(); }

auto Json::object_view() -> ObjectProxy& { return m_impl->refresh_object(); }
auto Json::object_view() const -> ObjectProxy const& { return m_impl->refresh_object(); }

std::string Json::serialized(std::size_t const indent_spacing) const {
	auto str = std::stringstream{};
	serialize_to(str, indent_spacing);
	return str.str();
}

std::ostream& Json::serialize_to(std::ostream& out, std::size_t indent_spacing) const { return Serializer{out, indent_spacing}(*this) << '\n'; }

bool Json::to_file(char const* path, std::size_t indent_spacing) const {
	auto file = std::ofstream(path);
	if (!file) { return false; }
	serialize_to(file, indent_spacing);
	return true;
}

template <typename T>
T Json::from_number(T const fallback) const {
	if (!m_impl) { return fallback; }
	if (auto const* payload = std::get_if<Impl::Number>(&m_impl->payload)) {
		auto ret = T{};
		if (detail::str_to_d(payload->serialized, ret)) { return ret; }
	}
	return fallback;
}

Json::IterBase& Json::IterBase::operator++() {
	++m_index;
	return *this;
}

Json& Json::ArrayIter::operator*() const {
	assert(m_impl && std::holds_alternative<Impl::Array>(m_impl->payload));
	auto& array = std::get<Impl::Array>(m_impl->payload);
	assert(m_index < array.size());
	return array[m_index];
}

auto Json::ObjectIter::operator*() const -> value_type {
	assert(m_impl && std::holds_alternative<Impl::Object>(m_impl->payload));
	auto& obj = std::get<Impl::Object>(m_impl->payload);
	assert(m_index < obj.keys.size() && m_index < obj.values.size());
	return {obj.keys[m_index], obj.values[m_index]};
}

bool Json::ArrayProxy::empty() const {
	return !m_impl || !std::holds_alternative<Impl::Array>(m_impl->payload) || std::get<Impl::Array>(m_impl->payload).empty();
}

std::size_t Json::ArrayProxy::size() const {
	if (!m_impl || !std::holds_alternative<Impl::Array>(m_impl->payload)) { return 0; }
	return std::get<Impl::Array>(m_impl->payload).size();
}

auto Json::ArrayProxy::begin() -> iterator { return array_begin(); }
auto Json::ArrayProxy::end() -> iterator { return array_end(); }
auto Json::ArrayProxy::begin() const -> const_iterator { return array_begin(); }
auto Json::ArrayProxy::end() const -> const_iterator { return array_end(); }

auto Json::ArrayProxy::array_begin() const -> ArrayIter {
	if (!m_impl || !std::holds_alternative<Impl::Array>(m_impl->payload)) { return {}; }
	return ArrayIter{m_impl, 0};
}

auto Json::ArrayProxy::array_end() const -> ArrayIter {
	if (!m_impl || !std::holds_alternative<Impl::Array>(m_impl->payload)) { return {}; }
	return ArrayIter{m_impl, std::get<Impl::Array>(m_impl->payload).size()};
}

bool Json::ObjectProxy::empty() const {
	return !m_impl || !std::holds_alternative<Impl::Object>(m_impl->payload) || std::get<Impl::Object>(m_impl->payload).keys.empty();
}

std::size_t Json::ObjectProxy::size() const {
	if (!m_impl || !std::holds_alternative<Impl::Object>(m_impl->payload)) { return 0; }
	return std::get<Impl::Object>(m_impl->payload).keys.size();
}

auto Json::ObjectProxy::begin() -> iterator { return object_begin(); }
auto Json::ObjectProxy::end() -> iterator { return object_end(); }
auto Json::ObjectProxy::begin() const -> const_iterator { return object_begin(); }
auto Json::ObjectProxy::end() const -> const_iterator { return object_end(); }

auto Json::ObjectProxy::object_begin() const -> ObjectIter {
	if (!m_impl || !std::holds_alternative<Impl::Object>(m_impl->payload)) { return {}; }
	return ObjectIter{m_impl, 0U};
}
auto Json::ObjectProxy::object_end() const -> ObjectIter {
	if (!m_impl || !std::holds_alternative<Impl::Object>(m_impl->payload)) { return {}; }
	return ObjectIter{m_impl, std::get<Impl::Object>(m_impl->payload).keys.size()};
}
} // namespace dj

std::string dj::to_string(Json const& json) { return json.serialized(); }
std::ostream& dj::operator<<(std::ostream& out, Json const& json) { return json.serialize_to(out); }
