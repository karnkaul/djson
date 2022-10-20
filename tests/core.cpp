#include <djson/json.hpp>
#include <test/test.hpp>
#include <algorithm>
#include <numeric>
#include <utility>

namespace {
bool eq(double const a, double const b) { return std::abs(a - b) < 0.0001; }

void default_is_null() {
	auto json = dj::Json{};
	EXPECT(!json);
	json = nullptr;
	EXPECT(!json);
}

void moved_is_null() {
	auto json = dj::Json{2};
	EXPECT(json.is_number());
	EXPECT(!json.is_null());
	auto moved = std::move(json);
	EXPECT(!json);
	EXPECT(!json.is_number());
	ASSERT(!moved.is_null());
	EXPECT(moved.is_number());
	EXPECT(moved.as_i64() == 2);
}

void boolean() {
	auto json = dj::Json{dj::true_v};
	EXPECT(json.is_bool());
	EXPECT(json.as_bool().value);
	json = dj::false_v;
	EXPECT(json.is_bool());
	EXPECT(!json.as_bool());
}

void integer() {
	auto json = dj::Json{42};
	EXPECT(json.is_number());
	EXPECT(json.as_i64() == 42);
	EXPECT(json.as_u64() == 42U);
	auto copy = json;
	EXPECT(copy.is_number());
	EXPECT(copy.as_i64() == json.as_i64());
	EXPECT(copy.as_u64() == json.as_u64());
	json = -42;
	EXPECT(json.is_number());
	EXPECT(json.as_i64() == -42);
	copy = json;
	EXPECT(copy.is_number());
	EXPECT(copy.as_i64() == json.as_i64());
	EXPECT(copy.as_u64() == json.as_u64());
}

void floating() {
	auto json = dj::Json{3.14};
	EXPECT(json.is_number());
	EXPECT(eq(json.as_double(), 3.14));
	auto copy = json;
	EXPECT(copy.is_number());
	EXPECT(eq(copy.as_double(), json.as_double()));
	json = dj::Json{-3.14};
	EXPECT(json.is_number());
	EXPECT(eq(json.as_double(), -3.14));
	copy = json;
	EXPECT(copy.is_number());
	EXPECT(eq(copy.as_double(), json.as_double()));
	json = dj::Json::parse("2.98e8");
	EXPECT(json.is_number());
	EXPECT(eq(json.as_double(), 2.98e8));
	json = dj::Json::parse("6.626e-34");
	EXPECT(json.is_number());
	EXPECT(eq(json.as_double(), 6.626e-34));
}

void empty_string() {
	auto json = dj::Json{""};
	EXPECT(json.is_string());
	EXPECT(json.as_string().empty());
	auto copy = json;
	EXPECT(copy.is_string());
	EXPECT(copy.as_string().empty());
}

void string() {
	auto json = dj::Json{"hello"};
	EXPECT(json.is_string());
	EXPECT(json.as_string() == "hello");
	auto copy = json;
	EXPECT(copy.is_string());
	EXPECT(copy.as_string() == json.as_string());
}

void escape_tab() {
	auto json = dj::Json{R"(hello\tworld)"};
	EXPECT(json.is_string());
	EXPECT(json.as_string() == "hello\tworld");
	auto copy = json;
	EXPECT(copy.is_string());
	EXPECT(copy.as_string() == json.as_string());
}

void escape_newline() {
	auto json = dj::Json{R"(hello\nworld)"};
	EXPECT(json.is_string());
	EXPECT(json.as_string() == "hello\nworld");
	auto copy = json;
	EXPECT(copy.is_string());
	EXPECT(copy.as_string() == json.as_string());
}

void escape_backslash() {
	auto json = dj::Json{R"(hello\\world)"};
	EXPECT(json.is_string());
	EXPECT(json.as_string() == "hello\\world");
	auto copy = json;
	EXPECT(copy.is_string());
	EXPECT(copy.as_string() == "hello\\world");
	json = R"(hello\\\\world)";
	EXPECT(json.as_string() == "hello\\\\world");
	copy = json;
	EXPECT(copy.as_string() == "hello\\\\world");
	auto const str = json.as_string();
	EXPECT(std::count_if(str.begin(), str.end(), [](char const c) { return c == '\\'; }) == 2);
}

void escape_quote() {
	auto json = dj::Json{R"(hello\"world)"};
	EXPECT(json.is_string());
	EXPECT(json.as_string() == "hello\"world");
	auto copy = dj::Json{R"(hello\"world)"};
	EXPECT(copy.is_string());
	EXPECT(copy.as_string() == json.as_string());
}

void make_array() {
	auto json = dj::Json{42};
	json.push_back(-1);
	json.push_back("1");
	EXPECT(json.is_array());
	EXPECT(json[0].is_number());
	EXPECT(json[0].as_i64() == -1);
	EXPECT(json[1].is_string());
	EXPECT(json[1].as_string() == "1");
}

void make_object() {
	auto json = dj::Json{42};
	json.assign("a", -1);
	json.assign("b", "1");
	auto test_obj = [](dj::Json const& obj) {
		EXPECT(obj.is_object());
		EXPECT(obj["a"].is_number());
		EXPECT(obj["a"].as_i64() == -1);
		EXPECT(obj["b"].is_string());
		EXPECT(obj["b"].as_string() == "1");
	};
	test_obj(json);
	auto copy = json;
	test_obj(copy);
}

void parse_array() {
	auto json = dj::Json::parse(R"(["a", true, 3.14, 42, null])");
	EXPECT(json.is_array());
	EXPECT(json.array_view().size() == 5);
	EXPECT(json[0].is_string());
	EXPECT(json[0].as_string() == "a");
	EXPECT(json[1].is_bool());
	EXPECT(json[1].as_bool().value);
	EXPECT(json[2].is_number());
	EXPECT(eq(json[2].as_double(), 3.14));
	EXPECT(json[3].is_number());
	EXPECT(json[3].as_i64() == 42);
	EXPECT(json[4].is_null());
}

void parse_object() {
	auto json = dj::Json::parse(R"({ "foo": { "a": "1", "b": 3.14, "c": -42 }, "bar": null })");
	EXPECT(json.is_object());
	EXPECT(json.object_view().size() == 2);
	auto const& foo = json["foo"];
	EXPECT(foo.is_object());
	EXPECT(foo.object_view().size() == 3);
	EXPECT(foo["a"].is_string());
	EXPECT(foo["a"].as_string() == "1");
	EXPECT(foo["b"].is_number());
	EXPECT(foo["b"].as_double() == 3.14);
	EXPECT(foo["c"].is_number());
	EXPECT(foo["c"].as_i64() == -42);
	EXPECT(json["bar"].is_null());

	json = dj::Json::parse(R"({ "description": "Debug shader code by \"printing\" any values of interest to the debug callback or stdout." })");
	EXPECT(!json.is_null());
}
} // namespace

int main() {
	auto runner = dj::test::Runner{};
	runner.add("default_is_null", &default_is_null);
	runner.add("moved_is_null", &moved_is_null);
	runner.add("boolean", &boolean);
	runner.add("integer", &integer);
	runner.add("floating", &floating);
	runner.add("empty_string", &empty_string);
	runner.add("string", &string);
	runner.add("escape_tab", &escape_tab);
	runner.add("escape_newline", &escape_newline);
	runner.add("escape_backslash", &escape_backslash);
	runner.add("escape_quote", &escape_quote);
	runner.add("make_array", &make_array);
	runner.add("make_object", &make_object);
	runner.add("parse_array", &parse_array);
	runner.add("parse_object", &parse_object);
	if (!runner.run()) { return EXIT_FAILURE; }
}
