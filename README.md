# KAssert: Karlsruhe Assertion Library for C++

![KaMPI.ng logo](./docs/images/logo.svg)

KAssert is the assertion library used by KaMPI.ng, the post-modern C++ wrapper for MPI. 
However, KAssert does not depend on MPI and can be used in non-MPI code.

## Features

- Assertion levels to distinguish between computationally cheap and expensive assertions
- Expression decomposition to give more insights into failed assertions 
- Throwing assertions

## Example 

The `KASSERT` macros accepts 1, 2 or 3 arguments. 

```
KASSERT(1 + 1 == 3, "The world is a lie!", kassert::normal);
KASSERT(1 + 1 == 3, "The world is a lie!"); // use default assertion level (kassert::normal)
KASSERT(1 + 1 == 3); // omit custom error message
```

Use `THROWING_KASSERT` to throw an exception if the assertion fails. 
This requires the library to be build in exception mode (`-DKASSERT_EXCEPTION_MODE=On`). 
If exception mode is not enabled, `THROWING_KASSERT` acts the same as `KASSERT`.

```
THROWING_KASSERT(1 + 1 == 3, "The world is a lie!");
THROWING_KASSERT(1 + 1 == 3); // omit custom error message
```

You can also throw a custom exception type using the `THROWING_KASSERT_SPECIFIED` macro:

```
THROWING_KASSERT_SPECIFIED(1 + 1 == 3, "The world is a lie!", your::ns::Exception [, ...]);
```

The constructor of your custom exception type must be called with a `std::string` as its first 
argument, followed by the remaining arguments `[, ...]` passed to `THROWING_KASSERT_SPECIFIED`. 

## Requirements

- C++17-ready compiler (GCC, Clang, ICX)
   
## LICENSE

KAssert is released under the GNU Lesser General Public License. See [COPYING](COPYING) and [COPYING.LESSER](COPYING.LESSER) for details.