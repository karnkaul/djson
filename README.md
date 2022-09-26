# djson v2.0

## Dumb and lightweight JSON parsing library.

[![Build Status](https://github.com/karnkaul/djson/actions/workflows/ci.yml/badge.svg)](https://github.com/karnkaul/djson/actions/workflows/ci.yml)

## Features

- First-in/first-out object maps
- Fast conversion of strings to numbers using `std::from_chars`
- Copiable `Json` objects
- Proxy containers for array/object iteration
- `std::string_view` interface to minimize redundant `std::string` copies
- Heterogenous arrays
- Escaped text (`\\`, `\"`, `\b`, `\t`, `\n`)
- Implicit construction for nulls, booleans, numbers, strings
- Serialization, pretty-print (default)
- Customization points for `from_json` and `to_json`
- Build tree from scratch

## Limitations

- Hex/binary literals not supported
- Only tested with single-byte characters

## Usage

`class Json` is the primary type around which the entire library's interface is designed. Use the static member function `Json::parse()` to attempt to parse text into a `Json` object:

```cpp
constexpr auto text = R"({ "foo": "party", "universe": 42 })";
auto const json = dj::Json::parse(text);
assert(!json.is_null());
```

Check if a key exists as an object via `contains()`. Access objects at keys via `operator[]`; non-existent keys return null:

```cpp
assert(json.contains("foo"));
auto const& foo = json["foo"];
auto const& universe = json["universe"];
```

Convert to a C++ type via `as_*()`:

```cpp
auto const foo_value = foo.as_string();
auto const universe_value = universe.as<int>();
assert(foo_value == "party" && universe_value == 42);
```

Iterate over arrays via `array_view()`:

```cpp
constexpr auto text = R"([ 1, 2, 3, 4, 5 ])";
auto const json = dj::Json::parse(text);
assert(json[1].as<int>() == 2);

auto const get_sum = [](dj::Json const& j) {
  int total{};
  for (auto const& i : j.array_view()) { total += i.as<int>(); }
  return total;
};
auto const sum = get_sum(json);
assert(sum == 15);
```

Iterate over objects via `object_view()` (iterators dereference to proxy objects). Check for keys via `contains()`:

```cpp
constexpr auto text = R"({ "Mercury": 0, "Venus": 0, "Earth": 1, "Mars": 2 })";
auto const json = dj::Json::parse(text);
assert(json.contains("Earth"));

auto moons = std::unordered_map<std::string, std::uint32_t>{};
for (auto const [key, count] : json.object_view()) { moons[std::string{key}] = count.as<std::uint32_t>(); }
assert(moons["Earth"] == 1);
```

Serialize to strings / output streams:

```cpp
std::string const serialized = to_string(json);
std::cout << json;
json.serialize_to(output_file, /*indent_spacing =*/ 0); // minify
```

Read from / write to files:

```cpp
auto const json = dj::Json::from_file("path/to/file.json");
// ...
if (json.to_file("path/to/file.json")) { /* success */ }
```

Parse your own types:

```cpp
namespace foo {
struct Item {
  std::string name{};
  int weight{};

  bool operator==(Item const&) const = default;
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

auto const src = foo::Item{
  .name = "Orb",
  .weight = 5,
};
auto json = dj::Json{};
to_json(json, src);
auto dst = foo::Item{};
from_json(json, dst);
assert(src == dst);
```

Declarations of all customization points (`from_json` and `to_json`) must be visible in all translation units that include `json.hpp`, else the program will be ill formed (ODR violation).

### Properties of class Json

- Lightweight (size of one pointer)
- Zero overhead default construction (does nothing)
- Supports move and copy semantics
- Moved-from objects can be reused
- Contains tree of itself
- Uses `std::from_chars<double>` and `std::make_unique_for_overwrite` if available
  - Falls back to `std::strtod` and `std::make_unique` if not (eg GCC 10)
- Proxy containers (returned by `.*_view()`) are non-copiable and non-moveable
  - They are intended for local scope usage only, storing them is unwise
  - Iterators propagate `Json` / container `const`-ness
- Each instance holds a variant (internal) of supported JSON types, including arrays and objects/maps
- There is no internal thread synchronization: 
  - Reading a `Json` from multiple threads is ok - operate on a `const` object to enforce this invariant
  - If any thread writes / uses (non-const) `operator[]`, external synchronziation will be required across _all_ threads

## Building

### Requirements

- CMake 3.18
- C++20 compiler (and stdlib)

### Integration

`djson` uses CMake, and supports the following workflows:

1. Clone/Fetch repo to subdirectory in project, use `add_subdirectory`
1. Build and install `djson` to some location, use `find_package(djson CONFIG)` and add install path to `CMAKE_PREFIX_PATH` during configuration
1. Copy sources, setup include paths, compiler options, build and install manually (not recommended)

Once integrated, use in any source file via:

```cpp
#include <djson/json.hpp>
```

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
