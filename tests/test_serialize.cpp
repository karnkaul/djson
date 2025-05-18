#include <djson/json.hpp>
#include <unit_test.hpp>
#include <print>

namespace {
using namespace dj;

constexpr auto no_spaces_v = [] {
	auto ret = SerializeOptions{};
	ret.flags |= SerializeFlag::NoSpaces;
	return ret;
}();

TEST(serialize_literals) {
	auto json = Json{};
	EXPECT(json.serialize() == "null\n");
	json.set_null();
	EXPECT(json.serialize() == "null\n");
	json.set_boolean(true);
	EXPECT(json.serialize() == "true\n");
	json.set_boolean(false);
	EXPECT(json.serialize() == "false\n");
	json.set_number(-42);
	EXPECT(json.serialize() == "-42\n");
	json.set_number(3.14);
	EXPECT(json.serialize() == "3.14\n");
	json.set_string("hello");
	EXPECT(json.serialize(SerializeOptions{.flags = serialize_flags_v & ~SerializeFlag::TrailingNewline}) == R"("hello")");

	auto ea = Json::empty_array().serialize();
	EXPECT(Json::empty_array().serialize() == "[]\n");
	EXPECT(Json::empty_object().serialize() == "{}\n");
}

TEST(serialize_array) {
	auto json = Json{};
	json.push_back(-42);
	json.push_back(true);
	json.push_back("hello");
	auto str = json.serialize();
	std::string_view expected = R"([
  -42,
  true,
  "hello"
]
)";
	std::println("serialized:\n{}", str);
	EXPECT(str == expected);

	expected = R"([-42,true,"hello"])";
	str = json.serialize(no_spaces_v);
	std::println("serialized: {}", str);
	EXPECT(str == expected);
}

TEST(serialize_object) {
	auto json = Json{};
	json.insert_or_assign("foo", -42);
	json.insert_or_assign("bar", "hello");
	auto str = json.serialize();
	std::string_view expected = R"({
  "bar": "hello",
  "foo": -42
}
)";
	std::println("serialized:\n{}", str);
	EXPECT(str == expected);

	expected = R"({"bar":"hello","foo":-42})";
	str = json.serialize(no_spaces_v);
	std::println("serialized: {}", str);
	EXPECT(str == expected);
}
} // namespace
