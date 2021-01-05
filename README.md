## Dumb JSON

[![Build status](https://ci.appveyor.com/api/projects/status/d236oefieo7mm9vx?svg=true)](https://ci.appveyor.com/project/karnkaul/djson)

This is a "dumb simple" JSON parsing library, it does not conform fully to any standards, it just works for my use cases.

## Features

- Variant-based tree schema
- Supports single-line comments via `#` or `//`
- Supports escaped text (eg, `\\` and `\"`)
- Supports building tree from scratch
- Quotes are optional for strings without spaces
- Configurable parse error actions: unwind, throw, terminate, ignore
- Configurable parse error logging callback
- Configurable serialisation options

## Limitations

- Not meant for large files / deep trees (`~std::shared_ptr<node_t>()` will smash the stack)
- Only tested with UTF-8 input
- Exponent/hex/binary/etc literals not supported (will be parsed as strings)

## Usage

### Requirements

- CMake 3.3
- C++17 compiler (and stdlib)

### Steps

1. Clone repo to appropriate subdirectory, say `dumb_json`
1. Add library to project via: `add_subdirectory(dumb_json)` and `target_link_libraries(foo djson::djson)`
1. Use via: `#include <dumb_json/djson.hpp>`

### Examples

**Reading JSON input**

`node_t::make(std::string_view)` attempts to parse the first top-level object from the passed string, while `node_t::read(std::string_view)` attempts to parse all the top-level objects. Both return a valid `node_t` per parsed object, which serves as the root node for that "document". Each `node_t` contains a `value_t`, which may hold other nodes via `node_ptr` (`std::shared_ptr<node_t>`).

```cpp
#include <dumb_json/djson.hpp>

// ...

std::string const json = "{foo: \"fu bar\\b\", # a comment\n\t arr: [1, 2]}";
if (auto n = dj::node_t::make(json)) {
  node_t& node = *n;
  std::string const foo = node["foo"].as<std::string>();  // fu ba
  std::vector<int> const arr = node["arr"].as<std::vector<int>>();  // {1, 2}
  // ...
}
```

Aliases for reading values (`as<T>()`):

- `array_fields_t<T>`: for reading array values (`std::vector<T>`),
- `map_fields_t<T>`: for reading object values (`std::unordered_map<std::string_view, T>`)
- `array_nodes_t`: array of nodes (`array_fields_t<node_ptr>`)
- `map_nodes_t` : map of nodes (`map_fields_t<node_ptr>`)

The subscript operator (`["foo"]`) behaves like stdlib maps, inserting a new element / replacing the existing one (and is thus not `const`). Use `get()` to throw if element is not present, `safe_get()` to return a default (invalid) node if not present (returns `node_t const&`), `find()` to search for a node (returns `node_t*`).

**Adding new data**

`node_t::set()` and `node_t::add()` can be used to add scalars (as strings) as either array or object values. `node_t::add()` overloads can be used to move existing nodes (as `node_ptr`s) as array or object values.

**Serialising data**

`node_t::serialise` (or `operator<<`) can be used to write to streams, `node_t::to_string()` to obtain a string. Customised `serial_opts_t` can be passed to each function (defined in `dumb_json/serial_opts.hpp`).

```cpp
dj::node_t arr;
arr.add("1");
arr.add("2");
dj::node_t doc;
doc.add("foo", "fu ba");
doc.add("arr", std::move(arr));
std::cout << doc;   // {"arr":[1,2],"foo":"fu ba"}
```

**Error Handling**

`g_error` (in `dumb_json/error_handler.hpp`) controls logging parsing errors and taking actions: call `g_error.set_callback()` with a custom function pointer (or `nullptr` to disable logging).

Set `DJSON_PARSE_ERROR_ACTION` in CMake (reflected in C++ as `error_handler_t::action`) to customise behaviour:

- `UNWIND` : abort parsing (default)
- `THROW` : throw `parse_error_t`
- `TERMINATE` : call `std::terminate()`
- `IGNORE` : keep parsing (do not use outside development/debugging)

Set `DJSON_PARSE_ERROR_OUTPUT` in CMake (reflected in C++ as `error_handler_t::output`) to customise default log output:

- `STDERR` : `stderr`
- `STDOUT` : `stdout`

### API Description

All public types (and configuration globals) are in `namespace dj`, with `dumb_json/djson.hpp` serving the central interface, and `dumb_json/serial_opts.hpp` and `dumb_json/error_handler.hpp` exposing configuration options.

`node_t` is the main usage class, representing a JSON `value`, whose variants are `scalar_t`, `field_map_t` (`object`), and `field_array_t` (`array`). Scalars are stored as the original string and lazily parsed into the target type on demand. `converter<T>::operator()(std::string_view scalar)` can be specialised to add custom scalar parsing rules.

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
