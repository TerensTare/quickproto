
# Quickproto

This repo contains the source code for a simple toy programming language written in C++23. The goal of this project is to study and understand how Sea of Nodes (SoN) can be used as an alternative to SSA for intermediate representation. Compilation goes through the following steps:

```cpp
Source code > Lexing > (Parsing + SoN generation + peephole/optimizing SoN) > Dead code elimination
```

Here is a list of the passes implemented in the peephole stage so far:
- Constant folding for the following nodes:
    - arithmetic: `+`, `-`, `*`, `/`
    - bit logic: `&`, `^`, `|`
    - unary: `-const(x)` to `const(-x)`
    - identity comparisons: `a == a` to `true`, `a != a` to `false`, same thing for `<`, `<=`, `>`, `>=`
- Strength reduction:
    - `a + a` is transformed into `a * 2`
- Memory reordering:
    - Two consecutive reads can be computed independently and reordered as needed
    - A read/write can be parallel to another write as long as they both do not touch the same location and index

## Building

### 📋 Prerequisites

- [ ] A C++23 compiler that supports `std::print` and `static operator()`.
- [ ] CMake 3.25 or later.
- [ ] [vcpkg](https://github.com/microsoft/vcpkg) to manage project dependencies. Make sure to set the `VCPKG_ROOT` environment variable as described in the official docs.


### 🔨 Building

```bash
mkdir build
cd build
# This is on Windows, depending on the OS/shell replace %VCPKG_ROOT% with how you access an environment variable
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build .
```