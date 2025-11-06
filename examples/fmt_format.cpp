// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/decimal/fmt_format.hpp>
#include <iostream>

#if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT) && defined(BOOST_DECIMAL_TEST_FMT)

int main()
{
    constexpr boost::decimal::decimal64_t val1 {"3.14"};
    constexpr boost::decimal::decimal32_t val2 {"3.141"};

    // The easiest is no specification which is general format
    // Given these values they will print in fixed format
    std::cout << "Default Format:\n";
    std::cout << fmt::format("{}", val1) << '\n';
    std::cout << fmt::format("{}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    std::cout << "Scientific Format:\n";
    std::cout << fmt::format("{:e}", val1) << '\n';
    std::cout << fmt::format("{:e}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    // Here this gives one digit of precision rounded according to current rounding mode
    std::cout << "Scientific Format with Specified Precision:\n";
    std::cout << fmt::format("{:.1e}", val1) << '\n';
    std::cout << fmt::format("{:.1e}", val2) << "\n\n";

    // This combines the padding modifier (10), precision (3 digits), and a type modifier (e)
    std::cout << "Scientific Format with Specified Precision and Padding:\n";
    std::cout << fmt::format("{:10.3e}", val1) << '\n';
    std::cout << fmt::format("{:10.3e}", val2) << '\n';

    return 0;
}

#else

int main()
{
    std::cout << "<fmt/format> is unsupported" << std::endl;
    return 0;
}

#endif
