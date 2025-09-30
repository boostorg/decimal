// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if __has_include(<decimal/decimal>)

#include <boost/decimal.hpp>
#include <boost/decimal/builtin_decimal32_t.hpp>
#include <boost/decimal/builtin_decimal64_t.hpp>
#include <boost/decimal/builtin_decimal128_t.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <cstring>

static constexpr std::size_t N {1024};

static std::mt19937_64 rng(42);

using namespace boost::decimal;

template <typename T>
void test_constructor()
{
    using bits = std::conditional_t<std::is_same<T, builtin_decimal32_t>::value, std::uint32_t,
                    std::conditional_t<std::is_same<T, builtin_decimal64_t>::value, std::uint64_t, detail::builtin_uint128_t>>;

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

        BOOST_TEST(signed_bits == unsigned_bits);
    }
}

template <typename T>
void test_comparisons()
{
    using software_type = std::conditional_t<std::is_same<T, builtin_decimal32_t>::value, decimal32_t,
                            std::conditional_t<std::is_same<T, builtin_decimal64_t>::value, decimal64_t, decimal128_t>>;

    std::uniform_int_distribution<long long> coeff_dist {LLONG_MIN, LLONG_MAX};
    std::uniform_int_distribution<int> exp_dist {-20, 20};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto lhs_coeff {coeff_dist(rng)};
        const auto rhs_coeff {coeff_dist(rng)};

        const auto lhs_exp {exp_dist(rng)};
        const auto rhs_exp {exp_dist(rng)};

        const software_type software_lhs {lhs_coeff, lhs_exp};
        const software_type software_rhs {rhs_coeff, rhs_exp};

        const T hardware_lhs {lhs_coeff, lhs_exp};
        const T hardware_rhs {rhs_coeff, rhs_exp};

        BOOST_TEST((software_lhs == software_rhs) == (hardware_lhs == hardware_rhs));
        BOOST_TEST((software_lhs != software_rhs) == (hardware_lhs != hardware_rhs));
        BOOST_TEST((software_lhs < software_rhs) == (hardware_lhs < hardware_rhs));
        BOOST_TEST((software_lhs <= software_rhs) == (hardware_lhs <= hardware_rhs));
        BOOST_TEST((software_lhs > software_rhs) == (hardware_lhs > hardware_rhs));
        BOOST_TEST((software_lhs >= software_rhs) == (hardware_lhs >= hardware_rhs));

        #ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR
        BOOST_TEST((software_lhs <=> software_rhs) == (hardware_lhs <=> hardware_rhs));
        #endif
    }
}

int main()
{
    test_constructor<builtin_decimal32_t>();
    test_constructor<builtin_decimal64_t>();
    test_constructor<builtin_decimal128_t>();

    test_comparisons<builtin_decimal32_t>();
    test_comparisons<builtin_decimal64_t>();
    test_comparisons<builtin_decimal128_t>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
