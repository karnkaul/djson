#include <iostream>
#include <span>
#include <sstream>
#include <djson/json.hpp>

using namespace std::string_view_literals;

#define ASSERT_EQ(expr) check(!!(expr), #expr, __func__, __LINE__)

namespace {
void check(bool result, char const* expr, char const* func, std::uint32_t line) {
	if (!result) {
		std::cerr << "[" << func << "] Assertion failed: " << expr << " (line " << line << ")\n";
		std::terminate();
	}
}

void empty() {
	auto json = dj::json{};
	ASSERT_EQ(json.is_null());
	auto copy = json;
	ASSERT_EQ(copy.is_null());
	ASSERT_EQ(json.is_null());
	auto move = std::move(json);
	ASSERT_EQ(move.is_null());
	ASSERT_EQ(json.is_null());
}

void bad_parse() {
	auto json = dj::json{};
	auto result = json.read(R"("bad",)");
	ASSERT_EQ(!result);
	result = json.read(R"(3.14.5)");
	ASSERT_EQ(!result);
	result = json.read(R"("foo": 3.14)");
	ASSERT_EQ(!result);
	auto text = R"({ "foo": 3.14.2 })";
	result = json.read(text);
	ASSERT_EQ(!result);
	text = R"(
{
    "foo": 3.14.2
}
	)";
	result = json.read(text);
	ASSERT_EQ(!result);
}

void literals() {
	auto json = dj::json{};
	auto result = json.read(R"("foo")");
	ASSERT_EQ(result && json.is_string());
	ASSERT_EQ(json == "foo"sv);
	auto copy = json;
	ASSERT_EQ(copy.is_string());
	ASSERT_EQ(copy == "foo"sv);

	result = json.read(R"(true)");
	ASSERT_EQ(result && json.is_boolean());
	ASSERT_EQ(json.as_boolean());
	copy = json;
	ASSERT_EQ(copy.is_boolean());
	ASSERT_EQ(copy.as_boolean());

	result = json.read(R"(42)");
	ASSERT_EQ(result && json.is_number());
	ASSERT_EQ(json.as_number<int>() == 42);
	copy = json;
	ASSERT_EQ(copy.is_number());
	ASSERT_EQ(copy.as_number<int>() == 42);

	result = json.read(R"(3.14)");
	ASSERT_EQ(result && json.is_number());
	ASSERT_EQ(std::abs(json.as_number<float>() - 3.14f) < 0.001f);
	copy = json;
	ASSERT_EQ(copy.is_number());
	ASSERT_EQ(std::abs(copy.as_number<float>() - 3.14f) < 0.001f);
}

void whitespace() {
	auto json = dj::json{};
	auto result = json.read(R"(

		42
	)");
	ASSERT_EQ(result && json.is_number());
	ASSERT_EQ(json.as<int>() == 42);

	result = json.read(R"(
		{
			"foo": "bar",
			"42": 42,
			"arr": [0, 1, 2]
		}
	)");
	ASSERT_EQ(result && json.is_object());
	ASSERT_EQ(json["foo"] == "bar"sv);
	ASSERT_EQ(json["42"].as_number<int>() == 42);
	ASSERT_EQ(json["arr"].is_array());
}

template <typename T, typename U>
constexpr bool range_cmp(T const& t, U const& u) {
	if (std::size(t) != std::size(u)) { return false; }
	for (std::size_t i = 0; i < std::size(t); ++i) {
		if (t[i] != u[i]) { return false; }
	}
	return true;
}

template <typename T>
std::vector<T> convert(std::vector<dj::ref<dj::json>> const& vec) {
	auto ret = std::vector<T>{};
	ret.reserve(vec.size());
	for (dj::json const& json : vec) { ret.push_back(json.as<T>()); }
	return ret;
}

void arrays() {
	auto json = dj::json{};
	auto result = json.read(R"([1, 2, 3])");
	ASSERT_EQ(result && json.is_array());
	ASSERT_EQ(range_cmp(std::array{1, 2, 3}, convert<int>(json.as_array())));
	auto copy = json;
	ASSERT_EQ(copy.is_array());
	ASSERT_EQ(range_cmp(std::array{1, 2, 3}, convert<int>(copy.as_array())));

	result = json.read(R"(
		[
			{
				"true": true
			},
			{
				"42": 42,
				"fubar": "fubar",
				"three": 3
			},
			{}
		]
	)");
	ASSERT_EQ(result && json.is_array());
	auto vec = json.as_array();
	ASSERT_EQ(vec.size() == 3);
	ASSERT_EQ(vec[0].get().is_object());
	ASSERT_EQ(vec[0].get()["true"].as_boolean());
	ASSERT_EQ(vec[1].get().is_object());
	ASSERT_EQ(vec[1].get()["42"].as_number<int>() == 42);
	ASSERT_EQ(vec[1].get()["fubar"] == "fubar"sv);
}

void tuples() {
	auto json = dj::json{};
	auto result = json.read(R"([true, "foo", 3.14])");
	ASSERT_EQ(result && json.is_array());
	auto arr = json.as_array();
	ASSERT_EQ(arr.size() == 3);
	ASSERT_EQ(arr[0].get().as_boolean());
	ASSERT_EQ(arr[1].get() == "foo"sv);
	ASSERT_EQ(std::abs(arr[2].get().as_number<float>() - 3.14f) < 0.001f);
	auto copy = json;
	arr = copy.as_array();
	ASSERT_EQ(arr.size() == 3);
	ASSERT_EQ(arr[0].get().as_boolean());
	ASSERT_EQ(arr[1].get() == "foo"sv);
	ASSERT_EQ(std::abs(arr[2].get().as_number<float>() - 3.14f) < 0.001f);
}

void ordering() {
	auto json = dj::json{};
	auto result = json.read(R"(
		{
			"42": 42,
			"fubar": "fubar",
			"three": 3
		}
	)");
	ASSERT_EQ(result && json.is_object());
	auto ovec = json.as_ordered_object();
	ASSERT_EQ(ovec[0].first == "42" && ovec[0].second.get().as_number<int>() == 42);
	ASSERT_EQ(ovec[1].first == "fubar" && ovec[1].second.get().as_string_view() == "fubar");
	ASSERT_EQ(ovec[2].first == "three" && ovec[2].second.get().as_number<int>() == 3);
}

void sample_htmlc() {
	auto json = dj::json{};
	static constexpr auto text = std::string_view(R"({
	"config": {
		"pathRoot": "views",
		"partials": "partials",
		"templates": "templates"
	},
	"fallbacks": {}
}
)");
	auto result = json.read(text);
	ASSERT_EQ(result);
	auto str = std::stringstream{};
	str << dj::serializer{json};
	auto strtext = str.str();
	bool eq = strtext == text;
	ASSERT_EQ(eq);
}

void serialize() {
	static constexpr auto text = R"({
	"42": 42,
	"fubar": "fubar",
	"nest": {
		"x": 1280,
		"y": 720
	},
	"three": 3
}
)"sv;
	auto json = dj::json{};
	auto result = json.read(text);
	ASSERT_EQ(result && json.is_object());
	auto str = json.serialize(false);
	ASSERT_EQ(str == dj::minify(text));
	str = json.serialize();
	ASSERT_EQ(str == text);
}
} // namespace

int main() {
	empty();
	bad_parse();
	literals();
	whitespace();
	arrays();
	tuples();
	ordering();
	sample_htmlc();
	serialize();
}
