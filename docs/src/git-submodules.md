# Current submodules

| Submodule | Upstream | CMake target |
|-----------|----------|--------------|
| `deps/glfw` | [glfw/glfw](https://github.com/glfw/glfw) | `glfw` |
| `deps/glm` | [g-truc/glm](https://github.com/g-truc/glm) | `glm` |
| `deps/tinyobjloader` | [tinyobjloader/tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | `tinyobjloader` |


# Git Submodules

All third-party dependencies live under `deps/` as git submodules. This keeps the repo self-contained without vendoring source code.

## Adding a new dependency

```bash
git submodule add https://github.com/<owner>/<repo>.git deps/<name>
```

For example, adding GLFW:

```bash
git submodule add https://github.com/glfw/glfw.git deps/glfw
```

Then wire it up in `CMakeLists.txt`:

```cmake
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
```

## Cloning a repo with submodules

```bash
# clone with all submodules at once
git clone --recurse-submodules <repo-url>

# or, if already cloned:
git submodule update --init --recursive
```

## Updating a submodule to latest

```bash
cd deps/glfw
git pull origin master
cd ../..
git add deps/glfw
git commit -m "deps: update glfw"
```

## Removing a submodule

```bash
git submodule deinit -f deps/<name>
git rm -f deps/<name>
rm -rf .git/modules/deps/<name>
```

## Gotcha: header-only vs submodule

Initially I used tinyobjloader as a header-only drop-in. Switched to a submodule so CMake handles include paths and we track a specific version. The trade-off: submodule adds a git dependency but keeps things cleaner when the library has its own `CMakeLists.txt`.
