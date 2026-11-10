# KAssert: Karlsruhe Assertion Library for C++
<p align="center">
   <img src="https://raw.githubusercontent.com/kamping-site/kassert/main/docs/images/logo.svg"\>
</p>

KAssert is the assertion library used by KaMPIng, the Karlsruhe MPI Wrapper.
However, KAssert does not depend on MPI and can be used in non-MPI code.
For more information, see the README below and the [documentation][].

## Features

- Assertion levels to distinguish between computationally cheap and expensive assertions
- Expression decomposition to give more insights into failed assertions
- Throwing assertions

## Example

The `KASSERT` macros accepts 1, 2 or 3 arguments.

```c++
KASSERT(1 + 1 == 3, "The world is a lie!", kassert::assert::normal);
KASSERT(1 + 1 == 3, "The world is a lie!"); // use default assertion level (kassert::assert::normal)
KASSERT(1 + 1 == 3); // omit custom error message
```

You can also use C++ streams to build custom error messages on the fly.

```c++
int a = 1;
int b = 3;
KASSERT(a + a == b, "Under the assumption that a is " << a << ", the world is a lie!");
```

Use `THROWING_KASSERT` to throw an exception if the assertion fails.
This requires the target property `KASSERT_EXCEPTION_MODE` to be set
to `ON` on the target linking against `kassert`.  If exception mode is
not enabled, `THROWING_KASSERT` acts the same as `KASSERT`.

```c++
THROWING_KASSERT(1 + 1 == 3, "The world is a lie!");
THROWING_KASSERT(1 + 1 == 3); // omit custom error message
```

You can also throw a custom exception type using the `THROWING_KASSERT_SPECIFIED` macro:

```c++
THROWING_KASSERT_SPECIFIED(1 + 1 == 3, "The world is a lie!", your::ns::Exception [, ...]);
```

The constructor of your custom exception type must be called with a `std::string` as its first
argument, followed by the remaining arguments `[, ...]` passed to `THROWING_KASSERT_SPECIFIED`.

### Assertion Levels

Assertions are enabled if their assertion level (optional third parameter of `KASSERT`) is **less than or equal to** the active assertion level.
Level `0` disables all assertions.
Set the CMake target property `KASSERT_ASSERTION_LEVEL` on your target linking against `kassert` to the numeric value of the desired assertion level:

```cmake
add_library(my_code ...)
target_link_libraries(my_code PRIVATE kassert::kassert)
set_target_properties(my_code PROPERTIES KASSERT_ASSERTION_LEVEL 10)
```

To set the level for all targets at once, set `KASSERT_DEFAULT_ASSERTION_LEVEL` before adding KAssert, e.g. `set(KASSERT_DEFAULT_ASSERTION_LEVEL $<IF:$<CONFIG:Debug>,30,10>)`.
The property and the default are numbers and may contain generator expressions; the property takes precedence.
If neither is set, the level defaults to `kassert::assert::normal` (30) and the compiler emits a warning.

### Custom Assertion Levels

You are free to define your own assertion levels. For instance:

```c++
namespace kamping::assert {
#define KAMPING_ASSERTION_LEVEL_LIGHT 20
constexpr int light = KAMPING_ASSERTION_LEVEL_LIGHT;

#define KAMPING_ASSERTION_LEVEL_HEAVY 40
constexpr int heavy = KAMPING_ASSERTION_LEVEL_HEAVY;
} // namespace kamping::assert
```

## Requirements

- C++17-ready compiler (GCC, Clang, ICX)

## Vendoring Support

KAssert can be embedded in other libraries with custom namespaces and macro names to prevent naming conflicts.
Each embedded copy is an independent *instance* with its own macros, header, CMake target and assertion level.

**When to vendor:** Use vendoring when developing a library that uses KAssert internally to avoid conflicts with user code that might also use KAssert.
Use one instance per library that is released and consumed on its own; components that are always released together can share one.

### Configuration Variables

- `KASSERT_VENDOR_ID` - Base identifier for deriving other variables
- `KASSERT_CMAKE_NAMESPACE` - Prefix for all CMake targets
- `KASSERT_CXX_NAMESPACE` - C++ namespace for the library
- `KASSERT_INCLUDE_SUBDIR` - Subdirectory for header installation
- `KASSERT_PREFIX` - The prefix for all macros and the assertion level property
- `KASSERT_DEFAULT_ASSERTION_LEVEL` - Assertion level for all targets linking to this instance which do not set the
  `<KASSERT_PREFIX>_ASSERTION_LEVEL` property; may contain generator expressions

### For Library Authors

```cmake
include(FetchContent)
FetchContent_Declare(mylib_kassert GIT_REPOSITORY https://github.com/kamping-site/kassert GIT_TAG v1.1.0)
block() # keep the KASSERT_* variables from leaking into other instances
    set(KASSERT_VENDOR_ID mylib)
    set(KASSERT_DEFAULT_ASSERTION_LEVEL $<IF:$<CONFIG:Debug>,30,10>)
    FetchContent_MakeAvailable(mylib_kassert)
endblock()

target_link_libraries(mylib INTERFACE mylib::kassert)
```

This creates the target `mylib::kassert` and the header `<mylib/kassert.hpp>`, which provides `MYLIB()`, `THROWING_MYLIB()` etc. instead of `KASSERT()`, in the namespace `mylib`.

- Set the default level of your users with `KASSERT_DEFAULT_ASSERTION_LEVEL`, not by adding `MYLIB_ASSERTION_LEVEL` to the compile definitions of your own targets.
  The default is attached to the KAssert target itself, so every target linking to the instance gets exactly one definition, no matter through how many of your targets it does so.
  Definitions on your own targets may be missing for users of only some of your targets and conflict for users of several.
- To make the level configurable, define a cache variable (e.g. `MYLIB_ASSERTION_LEVEL`) and pass its value as `KASSERT_DEFAULT_ASSERTION_LEVEL`.
- The first project that adds an instance defines it; later attempts to add an instance with the same CMake namespace are ignored, including their default level.
  So exactly one library should add each instance.
- If KAssert is installed with your library (`KASSERT_INSTALL`), the default level is part of the exported target.

### For Users of a Library Vendoring KAssert

Nothing needs to be done: targets linking to the library get the level its authors chose as default, without a warning.
To change it for a single target, set the `<KASSERT_PREFIX>_ASSERTION_LEVEL` property, e.g. `set_target_properties(my_app PROPERTIES MYLIB_ASSERTION_LEVEL 0)`.
To change it for all targets, use the configuration option of the library, if it provides one.

## LICENSE

KAssert is released under the MIT License. See [LICENSE](LICENSE) for details.

[documentation]: https://kamping-site.github.io/kassert/
