# C++23 Modules + `import std`

Upgraded from C++20 modules to C++23 with libc++ `import std;`. This eliminates the need to `#include` any STL headers — just `import std;` and you're done.

## What changed from C++20

The big win: **no more `#include` for standard library headers**. Before, every module needed a global module fragment stuffed with `#include <string>`, `#include <iostream>`, etc. Now it's just:

```cpp
export module greeting;

import std;

export void greet(const std::string& name) {
  std::println("Hello, {}!", name);
}
```

The global module fragment (`module;` ... `export module xxx;`) is still needed for non-standard headers like Vulkan and GLFW — just not for STL stuff anymore.

## CMake setup

This was the tricky part. `import std;` is still **experimental** in CMake, so there's a bunch of things that must be set **before** `project()`:

```cmake
cmake_minimum_required(VERSION 3.30)

# All of these must come BEFORE project() for toolchain detection
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "<uuid>")

project(myproject)

set(CMAKE_CXX_MODULE_STD ON)
```

### Why before `project()`?

CMake does toolchain detection during `project()`. It needs to know:

- `-stdlib=libc++` — so it finds libc++'s `std.cppm` module file
- `CMAKE_CXX_EXTENSIONS OFF` — so `std.pcm` is compiled with `-std=c++23` (not `-std=gnu++23`). If there's a mismatch, you get: `error: GNU extensions was enabled in precompiled file 'std.pcm' but is currently disabled`
- The experimental UUID — to unlock the feature gate

### The UUID situation

The `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID is a "gate" — CMake requires the exact UUID for your version to activate the feature. This is intentional: it forces you to acknowledge the feature is experimental.

**The UUID changes with every CMake version.** To find the right one:

```bash
strings $(which cmake) | grep -E '[0-9a-f]{8}-[0-9a-f]{4}-.*'
```

Known UUIDs:

| CMake version | UUID |
|---------------|------|
| 3.30 | `0e5b6991-d74f-4b3d-a41c-cf096e0b2508` |
| 4.0.x | `a9e1cf81-9932-4810-974b-6eccaf14e457` |
| 4.2.x | `d0edc3af-4c50-42ea-a356-e2862fe7a444` |

Once `import std` graduates from experimental, the UUID won't be needed anymore.

### `CMAKE_CXX_MODULE_STD`

This tells CMake to provide the `std` module target. Without it, `import std;` won't resolve. Set it after `project()`.

## clangd workaround

clangd doesn't understand `import std;` yet. It thinks standard library symbols are missing and auto-inserts `#include` directives on completion. Fix with `.clangd`:

```yaml
Diagnostics:
  MissingIncludes: None

Completion:
  HeaderInsertion: Never
```

`MissingIncludes: None` suppresses the "missing include" diagnostic, and `HeaderInsertion: Never` stops it from inserting `#include` when you accept a completion.

Downside: this also disables auto-insertion for non-standard headers (Vulkan, GLFW, etc.), so you'll need to add those manually.

## Writing a module (C++23 style)

For modules that only use the standard library — no global module fragment needed:

```cpp
export module greeting;

import std;

export void greet(const std::string& name) {
  std::println("Hello, {}!", name);
}
```

For modules that also use third-party headers, you still need the global module fragment:

```cpp
module;

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

export module app;

import std;

export class App { /* ... */ };
```

## Gotchas

1. **Order in CMakeLists.txt is critical** — `CMAKE_CXX_STANDARD`, `CMAKE_CXX_EXTENSIONS`, `-stdlib=libc++`, and the experimental UUID must all be set before `project()`. Getting this wrong gives confusing errors about `__CMAKE::CXX23` target not existing.

2. **Ninja is still required** — only the Ninja generator supports building module BMIs for the internal `__cmake_cxx23` target.

3. **Different machines need different UUIDs** — if the CMake version differs, the UUID must be updated. The `strings` trick above is the fastest way to find it.
