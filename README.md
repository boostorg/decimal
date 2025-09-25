# Decimal

|                  | Master                                                                                                                                                            |   Develop   |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------|
| Drone            | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/decimal/status.svg?ref=refs/heads/master)](https://drone.cpp.al/cppalliance/decimal)                 | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/decimal/status.svg?ref=refs/heads/develop)](https://drone.cpp.al/cppalliance/decimal) |
| Github Actions   | [![CI](https://github.com/cppalliance/decimal/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/cppalliance/decimal/actions/workflows/ci.yml) | [![CI](https://github.com/cppalliance/decimal/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/cppalliance/decimal/actions/workflows/ci.yml)
| Codecov          | [![codecov](https://codecov.io/gh/cppalliance/decimal/branch/master/graph/badge.svg?token=drvY8nnV5S)](https://codecov.io/gh/cppalliance/decimal)                 | [![codecov](https://codecov.io/gh/cppalliance/decimal/graph/badge.svg?token=drvY8nnV5S)](https://codecov.io/gh/cppalliance/decimal) |
| Fuzzing          | [![Fuzzing](https://github.com/cppalliance/decimal/actions/workflows/fuzz.yml/badge.svg?branch=master)](https://github.com/cppalliance/decimal/actions/workflows/fuzz.yml) | [![Fuzzing](https://github.com/cppalliance/decimal/actions/workflows/fuzz.yml/badge.svg?branch=develop)](https://github.com/cppalliance/decimal/actions/workflows/fuzz.yml) |
| Metal            | [![Metal](https://github.com/cppalliance/decimal/actions/workflows/metal.yml/badge.svg?branch=master)](https://github.com/cppalliance/decimal/actions/workflows/metal.yml) | [![Metal](https://github.com/cppalliance/decimal/actions/workflows/metal.yml/badge.svg?branch=develop)](https://github.com/cppalliance/decimal/actions/workflows/metal.yml) |

---

Decimal is an implementation of IEEE-754:2008 decimal floating point numbers.
See also [1].

The library is header-only, and requires C++14.
It is compatible through C++14, 17, 20, 23 and beyond.

# Notice

Decimal is under active development and is not an official boost library.

# How To Use The Library

This library is header only. It contains no other dependencies.
Simply `#include` it and use it.

## CMake

```sh
git clone https://github.com/cppalliance/decimal
cd decimal
mkdir build && cd build
cmake .. OR cmake .. -DCMAKE_INSTALL_PREFIX=/your/custom/path
cmake --build . --target install
```

then you can use `find_package(boost_decimal REQUIRED)`

## vcpkg

```sh
git clone https://github.com/cppalliance/decimal
cd decimal
vcpkg install decimal --overlay-ports=ports/decimal 
```

## Conan

```sh
git clone https://github.com/cppalliance/decimal
conan create decimal/conan --build missing
```

# Supported Platforms

Boost.Decimal is tested natively on Ubuntu (x86_64, s390x, and aarch64), macOS (x86_64, and Apple Silicon),
and Windows (x32 and x64); as well as emulated PPC64LE and ARM Cortex-M using QEMU with the following compilers:

* GCC 7 and later
* Clang 6 and later
* Visual Studio 2019 and later
* Intel OneAPI DPC++

# Synopsis

Decimal provides 3 IEEE-754 compliant types:

```cpp
namespace boost {
namespace decimal {

class decimal32_t;
class decimal64_t;
class decimal128_t;

} //namespace decimal
} //namespace boost
```

and also 3 similar but non-compliant types with improved runtime performance:

```cpp
namespace boost {
namespace decimal {

class decimal_fast32_t;
class decimal_fast64_t;
class decimal_fast128_t;

} //namespace decimal
} //namespace boost
```

These types operate like built-in floating point types.
They have their own implementations of the Standard-Library functions
(e.g. like those found in `<cmath>`, `<charconv>`, `<cstdlib>`, etc.).

The entire library can be conveniently included with `#include <boost/decimal.hpp>`

Using the decimal types is simple.

```cpp
#include <boost/decimal.hpp>
#include <iostream>

int main()
{
    using boost::decimal::decimal32_t;

    constexpr decimal32_t a {2, -1}; // Constructs the number 0.2
    constexpr decimal32_t b {1, -1}; // Constructs the number 0.1
    auto sum {a + b};

    std::cout << sum << std::endl; // prints 0.3

    const auto neg_a {2, -1, true}; // Constructs the number -0.2

    sum += neg_a;

    std::cout << sum << std::endl; // Prints 0.1

    return 0;
}
```

This intuitive straightforwardness is the same when using Standard-Library
functions (such as STL functions, `<cmath>` functions and the like).

```cpp
#include <boost/decimal.hpp>
#include <cassert>
#include <cstring>

int main()
{
    using namespace boost::decimal;

    decimal64_t val {-0.25}; // Construction from a double
    val = abs(val); // DO NOT call std::abs

    char buffer[256];
    auto r_to = to_chars(buffer, buffer + sizeof(buffer) - 1, val);
    assert(r_to); // checks std::errc()
    *r_to.ptr = '\0';

    decimal64_t return_value;
    auto r_from = from_chars(buffer, buffer + std::strlen(buffer), return_value);

    assert(val == return_value);

    return 0;
}
```

# Full Documentation

The complete documentation can be found at: https://develop.decimal.cpp.al

## References

- Michael F. Cowlishaw [_Floating-Point: Algorism(sic) for Computers_](https://www.cs.tufts.edu/~nr/cs257/archive/mike-cowlishaw/decimal-arith.pdf_Decimal), Proceedings of the 16th IEEE Symposium on Computer Arithmetic, 2003

- Donald E. Knuth, _The Art of Computer Programming Volume 2 Seminumerical Algorithms_, 3rd edition, 1998

- Jean-Michel Muller, _Elementary Functions_, 3rd edition, 2010

- Jean-Michel Muller, et. al., _Handbook of Floating-Point Arithmetic_, 2000

- John F. Hart, et. al.,  _Computer Approximations_, 1968

- IEEE, _IEEE Standard for Floating-Point Arithmetic_, 2019
