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
The default level is `kassert::assert::normal` (30).
Set the CMake target property `KASSERT_ASSERTION_LEVEL` on you target linking against `kassert` to the numeric value of the desired assertion level.
If omitted, the assertion level is set to `0`, which disables all assertions.

```cmake
add_library(my_code ...)
target_link_libraries(my_code PRIVATE kassert::kassert)
set_target_properties(my_code PROPERTIES KASSERT_ASSERTION_LEVEL 10)
```

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

## LICENSE

KAssert is released under the MIT License. See [LICENSE](LICENSE) for details.

[documentation]: https://kamping-site.github.io/kassert/
