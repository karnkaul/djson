#include <djson/json.hpp>
#include <test/test.hpp>

namespace temp {
struct Unit {
	std::string name{};
	int hp{};

	bool operator==(Unit const&) const = default;
};

struct Party {
	std::vector<Unit> units{};

	bool operator==(Party const&) const = default;
};

void from_json(dj::Json const& json, Unit& out, Unit const& fallback = {}) {
	from_json(json["name"], out.name, fallback.name);
	from_json(json["hp"], out.hp, fallback.hp);
}

void to_json(dj::Json& out, Unit const& unit) {
	to_json(out["name"], unit.name);
	to_json(out["hp"], unit.hp);
}

void from_json(dj::Json const& json, Party& out) {
	for (auto const& j : json.array_view()) {
		auto unit = Unit{};
		from_json(j, unit);
		out.units.push_back(std::move(unit));
	}
}

void to_json(dj::Json& out, Party const& party) {
	for (auto const& unit : party.units) {
		auto json = dj::Json{};
		to_json(json, unit);
		out.push_back(std::move(json));
	}
}
} // namespace temp

namespace {
void to_from(temp::Party const& src) {
	auto json = dj::Json{};
	to_json(json, src);
	auto dst = temp::Party{};
	from_json(json, dst);
	EXPECT(src == dst);
}
} // namespace

int main() {
	static auto const party = temp::Party{
		.units =
			{
				{.name = "monster", .hp = 50},
				{.name = "mage", .hp = 10},
				{.name = "tank", .hp = 30},
			},
	};
	auto runner = dj::test::Runner{};
	runner.add("to_from", [] { to_from(party); });
	if (!runner.run()) { return EXIT_FAILURE; }
}
