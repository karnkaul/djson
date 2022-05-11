# djson v1.2

## Dumb and lightweight JSON parsing library.

[![Build Status](https://github.com/karnkaul/djson/actions/workflows/ci.yml/badge.svg)](https://github.com/karnkaul/djson/actions/workflows/ci.yml)

## Features

- First-in/first-out object maps
- Fast conversion of strings to numbers (using `std::from_chars`)
- Copiable `json` objects
- Lightweight `array_view` and `object_view` "containers" for iteration
- Support to view strings stored internally via `std::string_view`s
- Supports heterogenous arrays
- Supports escaped text (`\\`, `\"`, `\b`, `\t`, `\n`)
- Supports building tree from scratch
- Serialization, pretty-print

## Limitations

- Hex/binary literals not supported
- Only tested with single-byte characters

## Usage

### Requirements

- CMake 3.17
- C++20 compiler (and stdlib)

### Quickstart

1. Clone/Fetch repo to appropriate subdirectory, say `djson`
1. Add library to project via: `add_subdirectory(djson)` and `target_link_libraries(foo djson::djson)`
1. Use via: `#include <djson/json.hpp>`

#### JSON payload: path/to.json

```json
{
  "text": "djson",
  "position": {
    "x": 200,
    "y": 500
  },
  "type": "opaque",
  "visible": true
}
```

#### Parse into custom struct

```cpp
struct vec2 {float x, y; };

struct custom_data {
  enum class type_t { opaque, transparent };

  std::string text{};
  vec2 position{};
  type_t type{};
  bool visible{};
};

// ...

auto json = dj::json{};
// read file and parse text
auto result = json.read("path/to.json");
if (!result) {
  // handle error
}
auto make_vec2 = [](json const& json, vec2 const fallback) {
  return vec2{json["x"].as_number<float>(fallback.x), json["y"].as_number<float>(fallback.y)};
};
auto data = custom_data{
  .text = json["text"], // implicit conversion to string
  .position = make_vec2(json["position"], {}),
  .type = json["type"].as_string_view() == "transparent" ? custom_data::type_t::transparent : custom_data::type_t::opaque,
  .visible = json["visible"].as_boolean(),
};
```

### API

#### Basics

`dj::json` models a JSON value, and is the "top of the tree": an instance owns all its leaf nodes. This is important for two reasons:
1. You should probably not use this with gargantuan JSONs: the stack might overflow / smash.
1. All parent instances must remain alive as long as their nodes are being examined.

```cpp
{
  auto root = dj::json{};
  // load value into root

  // use root within scope
}
// root and sub-jsons destroyed here
```

`json`s are natively copiable, though this rarely needed.

A single `json` instance may be one of the following `value_type`s (obtained via `.type()`), and the corresponding C++ types:
- `null`: no associated C++ type; default constructed `json`
- `boolean`: `bool`
- `number`: all integral and floating point types
- `string`: `std::string` (can also be viewed as `std::string_view`)
- `array`: internal type, can be viewed as `array_view`
- `object`: internal type, can be viewed as `object_view`

```cpp
dj::value_type type = json.type();
bool is_array = json.is_array();
```

`djson` never throws any exceptions (intentionally).

#### Subscripting

An `object` type `json` can be subscripted via `json["key"]`, which will return a (const) reference to the corresponding `json`. A particular key's presence can be checked for via `.contains(key)`. Keys will never be empty (or duplicated), but values can be. `Literal`s are `boolean`s, `number`s, and `string`s, which can be obtained directly via corresponding `.as_*()` functions.

```cpp
auto v = some_struct{
  json["key"], // implicit conversion to std::string / std::string_view
  json["boolean"].as_boolean(),
  json["int"].as_number<int>(42), // custom fallback
  json["position"]["x"].as<float>(),
  json["position"]["y"].as<float>(),
};
```

Accessing non-existent keys inserts a new null `json` associated with it, and returns a reference to it (like `std` maps); `const` overloads return a const reference to a local static null `json` instance.

#### Views and iteration

`array` and `object` types can be iterated over via `as_array()` / `as_object()`, which return an `array_view` / `object_view` respectively. Array views are like spans, where each iterator points to a `json`. Object view iterators dereference to a wrapper type instead: `std::pair<std::string_view, json&>`.

```cpp
for (auto& json : root.as_array()) {
  // ...
}

for (auto [key, json] : root.as_object()) { // note: not auto&
  // ...
}
```
An `array_view` can also be subscripted via `json[index]`, though this is generally not as useful. Out-of-bounds indices will return a null instance.

#### Serialization

`json` supports basic serialization via the `serializer` wrapper, which has one option: `pretty_print` - inserts spaces and indents. A `json` instance can also be set to a custom `Element` (`Literal` or another `json`) via `.set()` / `operator=`, which can be useful for building documents programmatically.

```cpp
json["key"] = "value";
std::cout << dj::serializer{json};
bool const written = dj::serializer{json, false}("path/to.json");
```

#### Misc

`djson` supports CMake install and generates versioned package config. In other words, it can be installed anywhere via CMake:

```
cmake --install /path/to/djson/build --prefix=/path/to/install
```

And located via `find_package(djson)` in another project in its own build-tree:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install
```

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
