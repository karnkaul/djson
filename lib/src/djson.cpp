module;

#include <cassert>

module djson;

import djson.detail;

namespace dj {
namespace fs = std::filesystem;

namespace detail {
auto IParser::create(std::unique_ptr<IValue> value) -> Json {
	auto ret = Json{};
	ret.m_value = std::move(value);
	return ret;
}
} // namespace detail

namespace {
template <JsonType T>
using value_payload_type = std::variant_alternative_t<std::size_t(T) - 1, detail::Value::Payload>;

static_assert(std::same_as<value_payload_type<JsonType::Boolean>, detail::literal::Bool>);
static_assert(std::same_as<value_payload_type<JsonType::Number>, detail::literal::Number>);
static_assert(std::same_as<value_payload_type<JsonType::String>, detail::literal::String>);
static_assert(std::same_as<value_payload_type<JsonType::Array>, detail::Array>);
static_assert(std::same_as<value_payload_type<JsonType::Object>, detail::Object>);

template <typename T>
[[nodiscard]] constexpr auto to_number(detail::literal::Number const in) {
	auto const visitor = [](auto const n) {
		return static_cast<T>(n);
	};
	return std::visit(visitor, in.payload);
}

[[nodiscard]] constexpr auto is_uppercase(char const c) { return c >= 'A' && c <= 'Z'; }

void to_lower(std::string& out) {
	static constexpr auto delta = static_cast<char>('a' - 'A');
	for (char& c : out) {
		if (!is_uppercase(c)) {
			continue;
		}
		c += delta;
	}
}

[[nodiscard]] auto is_jsonc_extension(fs::path const& path) {
	auto extension = path.extension().string();
	to_lower(extension);
	return extension == ".jsonc";
}

[[nodiscard]] auto resolve_symlink(fs::path path) -> fs::path {
	static constexpr auto max_iters_v{100};
	auto err = std::error_code{};
	for (auto iter = 0; iter < max_iters_v; ++iter) {
		if (fs::is_symlink(path, err)) {
			path = fs::read_symlink(path, err);
			if (err != std::errc{}) {
				return {};
			}
			continue;
		}

		return path;
	}
	return {};
}

[[nodiscard]] auto file_to_string(std::string_view const path, std::string& out) {
	if (path.empty()) {
		return false;
	}

	auto const real_path = resolve_symlink(path);
	auto err = std::error_code{};
	if (real_path.empty() || fs::is_directory(real_path, err) || fs::is_symlink(real_path)) {
		return false;
	}

	auto file = std::ifstream{real_path, std::ios::binary | std::ios::ate};
	if (!file.is_open()) {
		return false;
	}

	auto const size = file.tellg();
	file.seekg(0, std::ios::beg);
	out.resize(std::size_t(size));
	return !!file.read(out.data(), size);
}

[[nodiscard]] auto string_to_file(std::string_view const path, std::string_view const text) {
	if (path.empty()) {
		return false;
	}

	auto const fs_path = fs::path{path};
	auto err = std::error_code{};
	auto const parent_path = fs_path.parent_path();
	if (!parent_path.empty() && !fs::is_directory(parent_path, err)) {
		if (!fs::create_directories(parent_path, err)) {
			return false;
		}
	}

	auto file = std::ofstream{fs_path};
	return !!(file << text);
}

auto const empty_object_v = detail::Object{};
} // namespace

struct Json::Serializer {
	explicit Serializer(SerializeOptions const& options) : m_options(options) {}

	[[nodiscard]] auto operator()(Json const& json) -> std::string {
		process(json);
		if (!m_ret.empty()) {
			// pop last comma
			m_ret.pop_back();
			if (!is_set(Flag::NoSpaces) && is_set(Flag::TrailingNewline)) {
				m_ret.push_back('\n');
			}
		}
		return std::move(m_ret);
	}

  private:
	using Flag = SerializeFlag;

	[[nodiscard]] constexpr auto is_set(SerializeFlags const flag) const -> bool {
		return (m_options.flags & flag) == flag;
	}

	void process(dj::Json const& json) {
		if (json.is_null()) {
			append("null,");
			return;
		}

		auto const visitor = detail::Visitor{
			[this](detail::literal::Bool const b) {
				append("{},", b.value);
			},
			[this](detail::literal::Number const n) {
				std::visit(
					[this](auto const n) {
						append("{},", n);
					},
					n.payload);
			},
			[this](detail::literal::String const& s) {
				append(R"("{}",)", make_escaped(s.text));
			},
			[this](detail::Array const& a) {
				process_array(a);
			},
			[this](detail::Object const& o) {
				process_object(o);
			},
		};
		std::visit(visitor, static_cast<detail::Value&>(*json.m_value).payload);
	}

	void process_array(detail::Array const& array) {
		if (array.members.empty()) {
			m_ret.append("[],");
			return;
		}

		m_ret.push_back('[');
		++m_indents;
		for (auto const& json : array.members) {
			pre_next_value();
			process(json);
		}
		// pop last comma
		m_ret.pop_back();
		--m_indents;
		newline();
		m_ret.append("],");
	}

	void process_object(detail::Object const& object) {
		if (object.members.empty()) {
			m_ret.append("{},");
			return;
		}

		auto const sort_keys = is_set(Flag::SortKeys) && object.members.size() > 1;
		auto sorted_keys = std::vector<std::string_view>{};
		if (sort_keys) {
			sorted_keys.reserve(object.members.size());
			for (auto const& [key, _] : object.members) {
				sorted_keys.push_back(key);
			}
			std::ranges::sort(sorted_keys);
		}

		m_ret.push_back('{');
		++m_indents;

		if (sort_keys) {
			for (auto const key : sorted_keys) {
				auto const it = object.members.find(key);
				assert(it != object.members.end()); // NOLINT(misc-static-assert)
				subprocess_object(key, it->second);
			}
		} else {
			for (auto const& [key, value] : object.members) {
				subprocess_object(key, value);
			}
		}

		// pop last comma
		m_ret.pop_back();
		--m_indents;
		newline();
		m_ret.append("},");
	}

	void subprocess_object(std::string_view const key, Json const& value) {
		pre_next_value();
		append(R"("{}":)", key);
		space();
		process(value);
	}

	template <typename... Args>
	void append(std::format_string<Args...> fmt, Args&&... args) {
		std::format_to(std::back_inserter(m_ret), fmt, std::forward<Args>(args)...);
	}

	void space() {
		if (is_set(Flag::NoSpaces)) {
			return;
		}
		m_ret.push_back(' ');
	}

	void newline() {
		if (is_set(Flag::NoSpaces)) {
			return;
		}
		m_ret.append(m_options.newline);
		for (std::uint8_t i = 0; i < m_indents; ++i) {
			m_ret.append(m_options.indent);
		}
	}

	void pre_next_value() {
		if (m_ret.empty() || is_set(Flag::NoSpaces)) {
			return;
		}
		newline();
	}

	SerializeOptions const& m_options;

	std::string m_ret{};
	std::uint8_t m_indents{};
};

Json::Json(Json const& other) {
	if (other.m_value) {
		m_value = std::make_unique<detail::Value>(static_cast<detail::Value&>(*other.m_value));
	}
}

auto Json::operator=(Json const& other) -> Json& {
	if (&other != this) {
		if (!other.m_value) {
			m_value.reset();
		} else {
			ensure_impl();
			static_cast<detail::Value&>(*m_value) = static_cast<detail::Value&>(*other.m_value);
		}
	}
	return *this;
}

auto Json::parse(std::string_view const text, ParseMode const mode) -> Result<Json> {
	return detail::Parser{text, mode}.parse();
}

auto Json::from_file(std::string_view const path, ParseMode mode) -> Result<Json> {
	auto text = std::string{};
	if (!file_to_string(path, text)) {
		return std::unexpected(Error{.type = Error::Type::IoError});
	}

	if (mode == ParseMode::Auto && is_jsonc_extension(path)) {
		mode = ParseMode::Jsonc;
	}
	return parse(text, mode);
}

auto Json::empty_array() -> Json const& {
	static auto const ret = detail::Parser::make_json(detail::Array{});
	return ret;
}

auto Json::empty_object() -> Json const& {
	static auto const ret = detail::Parser::make_json(detail::Object{});
	return ret;
}

auto Json::get_type() const -> Type {
	if (m_value) {
		return Type(static_cast<detail::Value&>(*m_value).payload.index() + 1);
	}
	return Type::Null;
}

auto Json::as_bool(bool const fallback) const -> bool {
	if (!is_boolean()) {
		return fallback;
	}
	return std::get<detail::literal::Bool>(static_cast<detail::Value&>(*m_value).payload).value;
}

auto Json::as_double(double const fallback) const -> double {
	if (!is_number()) {
		return fallback;
	}
	return to_number<double>(std::get<detail::literal::Number>(static_cast<detail::Value&>(*m_value).payload));
}

auto Json::as_u64(std::uint64_t const fallback) const -> std::uint64_t {
	if (!is_number()) {
		return fallback;
	}
	return to_number<std::uint64_t>(std::get<detail::literal::Number>(static_cast<detail::Value&>(*m_value).payload));
}

auto Json::as_i64(std::int64_t const fallback) const -> std::int64_t {
	if (!is_number()) {
		return fallback;
	}
	return to_number<std::int64_t>(std::get<detail::literal::Number>(static_cast<detail::Value&>(*m_value).payload));
}

auto Json::as_string_view(std::string_view const fallback) const -> std::string_view {
	if (!is_string()) {
		return fallback;
	}
	return std::get<detail::literal::String>(static_cast<detail::Value&>(*m_value).payload).text;
}

auto Json::as_array() const -> std::span<Json const> {
	if (!is_array()) {
		return {};
	}
	return std::get<detail::Array>(static_cast<detail::Value&>(*m_value).payload).members;
}

auto Json::as_object() const -> StringTable<Json> const& {
	if (!is_object()) {
		return empty_object_v.members;
	}
	return std::get<detail::Object>(static_cast<detail::Value&>(*m_value).payload).members;
}

void Json::set_null() { m_value.reset(); }

void Json::set_boolean(bool const value) {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).payload = detail::literal::Bool{.value = value};
}

void Json::set_string(std::string_view const value) {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).payload = detail::literal::String{.text = std::string{value}};
}

void Json::set_number(std::int64_t const value) {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).payload = detail::literal::Number{.payload = value};
}

void Json::set_number(std::uint64_t const value) {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).payload = detail::literal::Number{.payload = value};
}

void Json::set_number(double const value) {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).payload = detail::literal::Number{.payload = value};
}

void Json::set_value(Json value) { m_value = std::move(value.m_value); }

void Json::set_array() {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).morph<detail::Array>().members.clear();
}

void Json::set_object() {
	ensure_impl();
	static_cast<detail::Value&>(*m_value).morph<detail::Object>().members.clear();
}

auto Json::push_back(Json value) -> Json& {
	ensure_impl();
	return static_cast<detail::Value&>(*m_value).morph<detail::Array>().members.emplace_back(std::move(value));
}

auto Json::insert_or_assign(std::string key, Json value) -> Json& {
	ensure_impl();
	auto& table = static_cast<detail::Value&>(*m_value).morph<detail::Object>().members;
	auto const [it, _] = table.insert_or_assign(std::move(key), std::move(value));
	assert(it != table.end()); // NOLINT(misc-static-assert)
	return it->second;
}

auto Json::at(std::string_view const key) const -> Json const& {
	if (!is_object()) {
		return detail::null_json_v;
	}
	auto const& object = std::get<detail::Object>(static_cast<detail::Value&>(*m_value).payload);
	auto const it = object.members.find(key);
	if (it == object.members.end()) {
		return detail::null_json_v;
	}
	return it->second;
}

auto Json::at(std::string_view const key) -> Json& {
	ensure_impl();
	auto& object = static_cast<detail::Value&>(*m_value).morph<detail::Object>();
	auto it = object.members.find(key);
	if (it == object.members.end()) {
		it = object.members.insert_or_assign(std::string{key}, Json{}).first;
	}
	assert(it != object.members.end()); // NOLINT(misc-static-assert)
	return it->second;
}

auto Json::at(std::size_t const index) const -> Json const& {
	if (!is_array()) {
		return detail::null_json_v;
	}
	auto const& array = std::get<detail::Array>(static_cast<detail::Value&>(*m_value).payload);
	if (index >= array.members.size()) {
		return detail::null_json_v;
	}
	return array.members.at(index);
}

auto Json::at(std::size_t const index) -> Json& {
	ensure_impl();
	auto& array = static_cast<detail::Value&>(*m_value).morph<detail::Array>();
	if (index >= array.members.size()) {
		array.members.resize(index + 1);
	}
	return array.members.at(index);
}

auto Json::serialize(SerializeOptions const& options) const -> std::string { return Serializer{options}(*this); }

auto Json::to_file(std::string_view const path, SerializeOptions const& options) const -> bool {
	if (path.empty()) {
		return false;
	}
	auto const text = serialize(options);
	return string_to_file(path, text);
}

void Json::ensure_impl() {
	if (m_value) {
		return;
	}
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	m_value.reset(new detail::Value);
}
} // namespace dj

auto dj::make_escaped(std::string_view const text) -> std::string {
	auto ret = std::string{};
	ret.reserve(text.size());
	for (char const c : text) {
		switch (c) {
		case '\"':
		case '\\': ret.push_back('\\'); break;
		case '\t': ret.append("\\t"); continue;
		case '\n': ret.append("\\n"); continue;
		case '\r': ret.append("\\r"); continue;
		default: break;
		}
		ret.push_back(c);
	}
	return ret;
}
