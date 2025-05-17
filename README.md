# djson v3.0

## Dumb and lightweight JSON parsing library.

[![Build Status](https://github.com/karnkaul/djson/actions/workflows/ci.yml/badge.svg)](https://github.com/karnkaul/djson/actions/workflows/ci.yml)

## Features

- Copiable `Json` objects
- Default construction (representing `null`) is "free"
- `as_string_view()`
- Heterogenous arrays
- Escaped text (`\\`, `\"`, `\b`, `\t`, `\n`)
- Implicit construction for nulls, booleans, numbers, strings
- Serialization, pretty-print (default)
- Customization points for `from_json` and `to_json`
- Build tree from scratch

### Limitations

- Escaped unicode (`\u1F604`) not currently supported

## Usage

### Input

`class Json` is the primary type around which the entire library's interface is designed. Use the static member function `Json::parse()` to attempt to parse text into a `Json` value. It returns a `Result` (ie, `std::expected<Json, Error>`):

```cpp
constexpr auto text = R"({
  "elements": [-2.5e3, "bar"],
  "foo": "party",
  "universe": 42
})";
auto result = dj::Json::parse(text);
auto& json = result.value();
assert(!json.is_null());
```

Access values of Objects via `operator[](key)`. A `const Json` will return `null` for non-existent keys, whereas a mutable `Json` will return a reference to a newly created value:

```cpp
auto const& elements = json["elements"];
auto const& foo = json["foo"];
auto const& universe = json["universe"];
assert(std::as_const(json)["nonexistent"].is_null());
```

Access values of Arrays via `operator[](index)`. A `const Json` will return `null` for out-of-bound indices, whereas a mutable `Json` will resize itself and return a reference to a newly created value:

```cpp
auto const& elem0 = elements[0];
auto const& elem1 = elements[1];
assert(elements[2].is_null());
```

Check the type of a `Json` value via `get_type()` / `is_*()`:

```cpp
assert(elements.get_type() == dj::JsonType::Array);
assert(elem0.get_type() == dj::JsonType::Number);
assert(elem1.is_string());
assert(foo.is_string());
assert(universe.is_number());
```

Convert to a C++ type via `as_*()` or `as<T>()`:

```cpp
assert(elem0.as_double() == -2500.0); // == on floats is just for exposition.
assert(elem1.as_string_view() == "bar");
assert(foo.as_string_view() == "party");
assert(universe.as<int>() == 42);
```

Iterate over Arrays via `as_array()`. Array elements are ordered:

```cpp
for (auto const [index, value] :
    std::views::enumerate(elements.as_array())) {
  std::println("[{}]: {}", index, value);
}

// output:
// [0]: -2500
// [1]: "bar"
```

Iterate over Objects via `as_object()`. Object elements are not ordered:

```cpp
for (auto const& [key, value] : json.as_object()) {
  std::println(R"("{}": {})", key, value);
}

// output (unordered):
// "universe": 42
// "foo": "party"
// "elements": [-2500,"bar"]
```

### Output

Use `set*()` to overwrite the value of a `Json` with a literal (`null` / boolean / number / string), an empty Array / Object, or another `Json` value. It can also be constructed this way:

```cpp
auto json = dj::Json{};
assert(json.is_null());
json.set_number(42);
assert(json.as<int>() == 42);
json.set_object();
assert(json.is_object());
assert(json.as_object().empty());
json.set_value(dj::Json::empty_array());
assert(json.is_array());
assert(json.as_array().empty());
json.set_value(dj::Json{true});
assert(json.as_bool());
```

### Serialization

Serialize to strings via `Json::serialize()` or `to_string()` or `std::format()` (and related). The first two can be customized via `SerializeOptions`. `std::formatter<Json>` uses `SerializeFlag::NoSpaces` for compact output.

```cpp
auto json = dj::Json::parse(R"({"foo": 42, "bar": [-5, true]})").value();
auto const options = dj::SerializeOptions{
  .indent = "\t",
  .newline = "\n",
  .flags = dj::SerializeFlag::SortKeys | dj::SerializeFlag::TrailingNewline,
};
auto const serialized = json.serialize(options);
std::print("{}", serialized);

/* output:
{
	"bar": [
		-5,
		true
	],
	"foo": 42
}
*/
```

Read from / write to files via `from_file()` / `to_file()`.

### Customization

Parse your own types:

```cpp
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

## Building

### Requirements

- CMake 3.24
- C++23 compiler (and stdlib)

### Integration

`djson` uses CMake, and expects to be built and linked as a static library. The suggested workflow is to use `FetchContent` or `git clone` / vendor the repository + use `add_subdirectory()`.

Once integrated, use in any source file via:

```cpp
#include <djson/json.hpp>
```

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
