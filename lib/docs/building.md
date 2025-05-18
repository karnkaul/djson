## Building

### Requirements

- CMake 3.24
- C++23 compiler (and stdlib)

### Integration

`djson` uses CMake, and expects to be built and linked as a static library. The suggested workflow is to use `FetchContent` or `git clone` / vendor the repository + use `add_subdirectory()`.

Once imported into the build tree, link to the `djson::djson` target and use via the primary header:

```cpp
#include <djson/json.hpp>
```
