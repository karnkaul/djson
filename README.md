## Dumb JSON

[![Build status](https://ci.appveyor.com/api/projects/status/d236oefieo7mm9vx?svg=true)](https://ci.appveyor.com/project/karnkaul/djson)

Dumb and lightweight JSON parsing library.

## Features

- Variant-based tree schema
- Supports escaped text (`\\`, `\"`, `\b`, `\t`, `\n`)
- Supports building tree from scratch
- Configurable serialization options

## Limitations

- Exponent/hex/binary literals not supported
- Only tested with UTF-8 input

## Usage

### Requirements

- CMake 3.3
- C++17 compiler (and stdlib)

### Steps

1. Clone repo to appropriate subdirectory, say `djson`
1. Add library to project via: `add_subdirectory(djson)` and `target_link_libraries(foo djson::djson)`
1. Use via: `#include <dumb_json/json.hpp>`

### Examples

**Reading JSON input**

`dj::json` is the main API class, representing a JSON value. `json::read()` attempts to parse a `std::string_view` into a value, and `json::load()` attempts to load the contents of a file and parse that. Both functions return a `json::result_t`, which contains errors if any, can be checked for success (returns `false` on IO/parse failure), and can be printed to a `std::ostream` object.

`json` offers several member functions to cast / traverse its value:

- `as<T>()`: cast JSON value to C++ type; valid `T`s:
  - `bool`, `arithmetic<T>`, `std::string` and `std::vector`s of each
  - `map_t`: cast as object type (key value pairs)
  - `vec_t`: cast as array type
- `contains(std::string)`: check if value is of object type and contains the passed key
- `contains(std::size_t)`: check if value is of array type and contains the passed index
- `find(std::string)`: search for the value corresponding to the passed key
- `find(std::size_t)`: search for the value corresponding to the passed index
- `operator[](std::string)`: get the value corresponding to the passed key (throws on failure)
- `operator[](std::size_t)`: get the value corresponding to the passed index (throws on failure)

```cpp
#include <dumb_json/json.hpp>

// ...

constexpr std::string_view str = "{\"foo\": \"fu bar\\b\", \n\t \"arr\": [1, 2]}";
dj::json json;
if (json.read(str)) {
  std::string const foo = json["foo"].as<std::string>(); // fu ba
  dj::json const& arr = json["arr"];
  std::vector<int> const vec = arr.as<std::vector<int>>(); // {1, 2}
  int const i = arr[0].as<int>(); // 2
  // ...
}
```

**Adding new data**

Use `json::set()` to set a value, `json::push_back()` to set as array type and push a value, and `json::insert()` to set as object type and associate a key/value.

**Serialising data**

`json::serialize` (or `operator<<`) can be used to write to streams, `json::to_string()` to obtain a string. Customized `serial_opts_t` can be passed to each function (defined in `dumb_json/serial_opts.hpp`).

```cpp
dj::json arr;
arr.push_back(1);
arr.push_back(2);
dj::json doc;
doc.insert("foo", "fu ba");
doc.insert("arr", std::move(arr));
std::cout << doc;   // {"arr":[1,2],"foo":"fu ba"}
```

### API Types

**JSON value**

- `json` (`std::variant<null_t, boolean_t, number_t, string_t, array_t, object_t>`)

**Value types**

- `null_t`
- `boolean_t` (`bool`)
- `number_t` (`std::string`)
- `string_t` (`std::string`)
- `array_t` (`std::vector<ptr<json>>`)
- `object_t` (`std::unordered_map<std::string, ptr<json>>`)

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
