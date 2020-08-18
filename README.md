## Dumb JSON

[![Build status](https://ci.appveyor.com/api/projects/status/d236oefieo7mm9vx?svg=true)](https://ci.appveyor.com/project/karnkaul/djson)

This is a "dumb simple" JSON parsing library, it does not follow/meet any standards, it just works for my use cases.

### Features

- Data types:
  - Value types: `boolean`, `integer`, `floating`, `string`
  - Container types; `object`, `array`
- Nested container types
- Parse serialised text into `object`
- Add custom fields to `object`
- Serialise an `object`, optionally sorting all keys
- Escaped text (eg, `\\` and `\"`)

### Usage

**Requirements**

- Windows / Linux (any architecture)
- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `dumb_json`
1. Add library to project via: `add_subdirectory(dumb_json)` and `target_link_libraries(foo djson)`
1. Use via: `#include <dumb_json/dumb_json.hpp>`

### Contributing

Pull/merge requests are welcome.
