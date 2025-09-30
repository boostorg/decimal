// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if __has_include(<decimal/decimal>) && (__cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))

#include <boost/decimal.hpp>
#include <boost/decimal/builtin_decimal32_t.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <cstring>

inline constexpr std::size_t N {1024};

static std::mt19937_64 rng(42);

using namespace boost::decimal;

template <typename T>
void test_constructor()
{
    using bits = std::conditional_t<std::is_same_v<T, builtin_decimal32_t>, std::uint32_t, std::uint64_t>;
    static_assert(sizeof(bits) == sizeof(T), "Wrong size for memcpy");

    std::uniform_int_distribution<long long> dist {LLONG_MIN, LLONG_MAX};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto coeff {dist(rng)};
        const auto unsigned_coeff {detail::make_positive_unsigned(coeff)};

        const T signed_decimal {coeff, 0};
        const T unsigned_decimal {unsigned_coeff, 0, coeff < 0};

        bits signed_bits;
        std::memcpy(&signed_bits, &signed_decimal, sizeof(signed_decimal));

        bits unsigned_bits;
        std::memcpy(&unsigned_bits, &unsigned_decimal, sizeof(unsigned_decimal));

        BOOST_TEST_EQ(signed_bits, unsigned_bits);
    }
}

int main()
{
    test_constructor<builtin_decimal32_t>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
