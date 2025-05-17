#include <djson/json.hpp>
#include <unit_test.hpp>
#include <print>
#include <ranges>

namespace {
TEST(json_input) {
	constexpr auto text = R"({
  "elements": [-2.5e3, "bar"],
  "foo": "party",
  "universe": 42
})";
	auto result = dj::Json::parse(text);
	auto& json = result.value();
	EXPECT(!json.is_null());

	auto const& elements = json["elements"];
	auto const& foo = json["foo"];
	auto const& universe = json["universe"];
	EXPECT(std::as_const(json)["nonexistent"].is_null());

	auto const& elem0 = elements[0];
	auto const& elem1 = elements[1];
	EXPECT(elements[2].is_null());

	EXPECT(elements.get_type() == dj::JsonType::Array);
	EXPECT(elem0.get_type() == dj::JsonType::Number);
	EXPECT(elem1.is_string());
	EXPECT(foo.is_string());
	EXPECT(universe.is_number());

	EXPECT(elem0.as_double() == -2500.0);
	EXPECT(elem1.as_string_view() == "bar");
	EXPECT(foo.as_string_view() == "party");
	EXPECT(universe.as<int>() == 42);

	for (auto const [index, value] : std::views::enumerate(elements.as_array())) { std::println("[{}]: {}", index, value); }

	for (auto const& [key, value] : json.as_object()) { std::println(R"("{}": {})", key, value); }
}

TEST(json_output) {
	auto json = dj::Json{};
	EXPECT(json.is_null());
	json.set_boolean(true);
	EXPECT(json.as_bool());
	json.set_number(42);
	EXPECT(json.as<int>() == 42);
	json.set_string("meow");
	EXPECT(json.as_string_view() == "meow");
	json.set_object();
	EXPECT(json.is_object());
	EXPECT(json.as_object().empty());
	json.set_value(dj::Json::empty_array());
	EXPECT(json.is_array());
	EXPECT(json.as_array().empty());
	json.set_value(dj::Json{true});
	EXPECT(json.as_bool());
}

TEST(json_serialize) {
	auto json = dj::Json::parse(R"({"foo": 42, "bar": [-5, true]})").value();
	auto const options = dj::SerializeOptions{
		.indent = "\t",
		.newline = "\n",
		.flags = dj::SerializeFlag::SortKeys | dj::SerializeFlag::TrailingNewline,
	};
	auto const serialized = json.serialize(options);
	static constexpr std::string_view expected_v = R"({
	"bar": [
		-5,
		true
	],
	"foo": 42
}
)";
	EXPECT(serialized == expected_v);
	std::print("{}", serialized);
}
namespace foo {
struct Item {
	std::string name{};
	int weight{};

	auto operator==(Item const&) const -> bool = default;
};

void from_json(dj::Json const& json, Item& out) {
	from_json(json["name"], out.name);
	from_json(json["weight"], out.weight);
}

void to_json(dj::Json& out, Item const& item) {
	to_json(out["name"], item.name);
	to_json(out["weight"], item.weight);
}
} // namespace foo

TEST(json_customize) {
	auto const src = foo::Item{
		.name = "Orb",
		.weight = 5,
	};
	auto json = dj::Json{};
	to_json(json, src);
	auto dst = foo::Item{};
	from_json(json, dst);
	EXPECT(src == dst);
}
} // namespace
