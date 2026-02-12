---
title: 'Boost.Decimal: A C++14 Library for Decimal Floating Point Arithmetic'
tags:
  - C++
authors:
  - name: Matt Borland
    orcid: 0009-0005-8183-4254
    equal-contrib: true
    affiliation: 1
  - name: Christopher Kormanyos
    equal-contrib: true
    affiliation: 2
affiliations:
 - name: The C++ Alliance
   index: 1
 - name: Independent Researcher, Germany
   index: 2
date: 04 February 2025
bibliography: paper.bib
---

# Summary

Boost.Decimal is a header-only, C++14 implementation of IEEE 754 decimal floating-point arithmetic types (`decimal32_t`, `decimal64_t` and `decimal128_t`), integrating seamlessly with the C++ Standard Template Library and other Boost Libraries [@boost].
The library is available at [https://github.com/boostorg/decimal](https://github.com/boostorg/decimal), and in the Boost distribution with most standard package managers starting from version 1.91.

# Statement of need

In the 2008 revision of IEEE 754 [@IEEE754:2008], decimal floating-point arithmetic was standardized.
These types are critical in applications where decimal numbers must be represented exactly, such as financial software [@Cowlishaw:2003].
Binary floating-point representations cannot precisely store common decimal values like 0.1, leading to subtle rounding errors that accumulate over repeated calculations.
Prior to Boost.Decimal, no cross-platform C++ library provided IEEE 754 decimal arithmetic or the associated types.

# State of the field

The two main implementations in this space are produced by IBM [@ibm_libdfp] and Intel [@intel_dfp], with the latter representing the industry standard.
These libraries have two shortcomings that Boost.Decimal aims to address. First, both libraries are fundamentally C libraries.
This can make integration into modern C++ applications awkward, and they lack support for C++-specific functionality provided in the Standard Library. 
Second, neither library is cross-platform; for instance, neither library supports ARM architectures.
Boost.Decimal addresses both of these shortcomings.

# Software design

Boost.Decimal's design philosophy emphasizes ease of use, portability, and performance.

To make the library easy to consume, it requires only C++14 and is header-only with zero external dependencies.
Users can simply clone the repository, add `#include <boost/decimal.hpp>`, and begin using the library, even with older toolchains such as `clang++-6`.
Portability is a primary design concern, so we test the library on all three major operating system families (Linux, Windows, and macOS) across a variety of architectures including: x86-32, x86-64, ARM32, ARM64, S390X, and PPC64LE.
Our continuous integration pipeline ensures consistent numerical results across all supported platforms.
Performance is also a priority, both in absolute terms and relative to existing libraries.
[Extensive benchmarking](https://develop.decimal.cpp.al/benchmarks.html) demonstrates that Boost.Decimal outperforms both the Intel and IBM libraries in many operations across a variety of platforms.
Benchmark results and methodology are available in the repository documentation.

A minimal usage example showing the representation difference between decimal and binary floating point arithmetic:

```c++
#include <boost/decimal.hpp>    // This includes the entire decimal library
#include <iostream>
#include <iomanip>

int main()
{
    using namespace boost::decimal::literals; // The literals are in their own namespace like std::literals

    // First we show the result of 0.1 + 0.2 using regular doubles
    std::cout << std::fixed << std::setprecision(17)
              << "Using doubles:\n"
              << "0.1 + 0.2 = " << 0.1 + 0.2 << "\n\n";

    // Construct the two decimal values
    // We construct using the literals defined by the library
    constexpr boost::decimal::decimal64_t a {0.1_DD};
    constexpr boost::decimal::decimal64_t b {0.2_DD};

    // Now we display the result of the same calculation using decimal64_t
    std::cout << "Using decimal64_t:\n"
              << "0.1_DD + 0.2_DD = " << a + b << std::endl;
}
```
And the expected output is:
```
Using doubles:
0.1 + 0.2 = 0.30000000000000004

Using decimal64_t:
0.1_DD + 0.2_DD = 0.3000000000000000
```

# Research Impact Statement

Boost.Decimal enables reproducible numerical research in domains where decimal precision is essential. 
The library's cross-platform consistency ensures that computational results are identical regardless of hardware architecture, a critical property for scientific reproducibility.

Quantitative finance research frequently involves calculations on currency values, interest rates, and transaction amounts that are inherently decimal. 
Binary floating-point approximations can introduce systematic biases in Monte Carlo simulations, risk calculations, and backtesting frameworks. 
Boost.Decimal allows researchers to eliminate these artifacts and produce results that match real-world financial systems.

Researchers investigating floating-point error accumulation can use Boost.Decimal as a reference implementation to compare against binary floating-point results. 
This facilitates studies on when decimal arithmetic provides meaningful accuracy improvements over binary representations. 
Additionally, unlike existing implementations, Boost.Decimal runs on ARM and other non-x86 architectures, enabling research on decimal arithmetic performance and applications in embedded systems and other platforms.

# AI usage disclosure

No generative AI tools were used in the development of this software, the writing
of this manuscript, or the preparation of supporting materials.

# Acknowledgements

We acknowledge The C++ Alliance for sponsoring the development of this work.
The library underwent peer review in January and October 2025 by domain experts before acceptance into the Boost library collection.
We thank all of these reviewers for their time and contributions.

# References
