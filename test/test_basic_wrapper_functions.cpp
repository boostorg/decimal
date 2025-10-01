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

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127)
#endif

template <typename T>
void test_addition()
{
    std::uniform_int_distribution<long long> dist {-10000, 10000};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto lhs_val {dist(rng)};
        const auto rhs_val {dist(rng)};

        T decimal_lhs {lhs_val};
        T decimal_rhs {rhs_val};

        BOOST_TEST(decimal_lhs + rhs_val == lhs_val + decimal_rhs);
        BOOST_TEST_EQ(static_cast<long long>(decimal_lhs + rhs_val), static_cast<long long>(lhs_val + decimal_rhs));

        decimal_lhs += rhs_val;
        decimal_rhs += lhs_val;

        BOOST_TEST(decimal_lhs == decimal_rhs);

        // Mixed type
        const builtin_decimal128_t big_lhs {lhs_val};
        const builtin_decimal128_t big_rhs {rhs_val};

        auto big_res_rhs {decimal_lhs + big_rhs};
        auto big_res_lhs {big_lhs + decimal_rhs};

        static_assert(std::is_same<decltype(big_res_lhs), builtin_decimal128_t>::value, "Wrong output type");
        static_assert(std::is_same<decltype(big_res_rhs), builtin_decimal128_t>::value, "Wrong output type");

        decimal_lhs += big_rhs;
        decimal_rhs += big_lhs;

        BOOST_TEST(big_res_lhs == decimal_rhs);
        BOOST_TEST(big_res_rhs == decimal_lhs);
    }
}

template <typename T>
void test_subtraction()
{
    std::uniform_int_distribution<long long> dist {-10000, 10000};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto lhs_val {dist(rng)};
        const auto rhs_val {dist(rng)};

        T decimal_lhs {lhs_val};
        T decimal_rhs {rhs_val};

        BOOST_TEST(decimal_lhs - rhs_val == lhs_val - decimal_rhs);
        BOOST_TEST_EQ(static_cast<long long>(decimal_lhs - rhs_val), static_cast<long long>(lhs_val - decimal_rhs));

        decimal_lhs -= rhs_val;
        decimal_rhs = lhs_val - decimal_rhs;

        BOOST_TEST(decimal_lhs == decimal_rhs);

        // Mixed type
        const builtin_decimal128_t big_rhs {rhs_val};

        auto big_res_rhs {decimal_lhs - big_rhs};

        static_assert(std::is_same<decltype(big_res_rhs), builtin_decimal128_t>::value, "Wrong output type");

        decimal_lhs -= big_rhs;
        BOOST_TEST(big_res_rhs == decimal_lhs);
    }
}

template <typename T>
void test_multiplication()
{
    std::uniform_int_distribution<long long> dist {-100, 100};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto lhs_val {dist(rng)};
        const auto rhs_val {dist(rng)};

        T decimal_lhs {lhs_val};
        T decimal_rhs {rhs_val};

        BOOST_TEST(decimal_lhs * rhs_val == lhs_val * decimal_rhs);
        BOOST_TEST_EQ(static_cast<long long>(decimal_lhs * rhs_val), static_cast<long long>(lhs_val * decimal_rhs));

        decimal_lhs *= rhs_val;
        decimal_rhs *= lhs_val;

        BOOST_TEST(decimal_lhs == decimal_rhs);

        // Mixed type
        const builtin_decimal128_t big_lhs {lhs_val};
        const builtin_decimal128_t big_rhs {rhs_val};

        auto big_res_rhs {decimal_lhs * big_rhs};
        auto big_res_lhs {big_lhs * decimal_rhs};

        static_assert(std::is_same<decltype(big_res_lhs), builtin_decimal128_t>::value, "Wrong output type");
        static_assert(std::is_same<decltype(big_res_rhs), builtin_decimal128_t>::value, "Wrong output type");

        decimal_lhs *= big_rhs;
        decimal_rhs *= big_lhs;

        BOOST_TEST(big_res_lhs == decimal_rhs);
        BOOST_TEST(big_res_rhs == decimal_lhs);
    }
}

template <typename T>
void test_division()
{
    std::uniform_int_distribution<long long> dist {-10000, 10000};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto lhs_val {dist(rng)};
        const auto rhs_val {dist(rng)};

        T decimal_lhs {lhs_val};
        T decimal_rhs {rhs_val};

        BOOST_TEST(decimal_lhs / rhs_val == lhs_val / decimal_rhs);
        BOOST_TEST_EQ(static_cast<long long>(decimal_lhs / rhs_val), static_cast<long long>(lhs_val / decimal_rhs));

        decimal_lhs /= rhs_val;
        decimal_rhs = lhs_val / decimal_rhs;

        BOOST_TEST(decimal_lhs == decimal_rhs);

        // Mixed type
        const builtin_decimal128_t big_rhs {rhs_val};

        auto big_res_rhs {decimal_lhs / big_rhs};
        static_cast<void>(big_res_rhs);

        static_assert(std::is_same<decltype(big_res_rhs), builtin_decimal128_t>::value, "Wrong output type");

        decimal_rhs = static_cast<T>(decimal_lhs / big_rhs);
        decimal_lhs /= big_rhs;

        BOOST_TEST(decimal_lhs == decimal_rhs);
    }
}

template <typename T>
void test_increment_decrement()
{
    std::uniform_int_distribution<long long> dist {-10000, 10000};

    for (std::size_t i = 0; i < N; ++i)
    {
        const auto val {dist(rng)};
        T lhs {val - 10};
        builtin_decimal128_t rhs {val + 10};

        for (std::size_t j {}; j < 5u; ++j)
        {
            ++lhs;
            const auto current_lhs {lhs++};
            --rhs;
            const auto current_rhs {rhs--};

            if (j != 4u)
            {
                BOOST_TEST(current_lhs != current_rhs);
            }
        }

        BOOST_TEST(lhs == static_cast<T>(rhs));
        BOOST_TEST(lhs == T{rhs});
    }
}

template <typename T>
void test_limits_comparisons();

template <>
void test_limits_comparisons<builtin_decimal32_t>()
{
    static_assert(std::numeric_limits<builtin_decimal32_t>::digits == std::numeric_limits<decimal32_t>::digits, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::digits10 == std::numeric_limits<decimal32_t>::digits10, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::max_digits10 == std::numeric_limits<decimal32_t>::max_digits10, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::radix == std::numeric_limits<decimal32_t>::radix, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::min_exponent == std::numeric_limits<decimal32_t>::min_exponent, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::min_exponent10 == std::numeric_limits<decimal32_t>::min_exponent10, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::max_exponent == std::numeric_limits<decimal32_t>::max_exponent, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::max_exponent10 == std::numeric_limits<decimal32_t>::max_exponent10, "Should Match");
    static_assert(std::numeric_limits<builtin_decimal32_t>::tinyness_before == std::numeric_limits<decimal32_t>::tinyness_before, "Should Match");
}

template <typename T>
void test_limits()
{
    static_assert(std::numeric_limits<T>::is_specialized, "Wrong");
    static_assert(std::numeric_limits<T>::is_signed, "Wrong");
    static_assert(!std::numeric_limits<T>::is_integer, "Wrong");
    static_assert(!std::numeric_limits<T>::is_exact, "Wrong");
    static_assert(std::numeric_limits<T>::has_infinity, "Wrong");
    static_assert(std::numeric_limits<T>::has_quiet_NaN, "Wrong");
    static_assert(std::numeric_limits<T>::has_signaling_NaN, "Wrong");
    static_assert(std::numeric_limits<T>::round_style == std::round_indeterminate, "Wrong");

    static_assert(std::numeric_limits<T>::is_iec559, "Wrong");
    static_assert(std::numeric_limits<T>::is_bounded, "Wrong");
    static_assert(!std::numeric_limits<T>::is_modulo, "Wrong");

    test_limits_comparisons<T>();
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

int main()
{
    test_constructor<builtin_decimal32_t>();
    test_constructor<builtin_decimal64_t>();
    test_constructor<builtin_decimal128_t>();

    test_comparisons<builtin_decimal32_t>();
    test_comparisons<builtin_decimal64_t>();
    test_comparisons<builtin_decimal128_t>();

    test_addition<builtin_decimal32_t>();
    test_addition<builtin_decimal64_t>();
    test_addition<builtin_decimal128_t>();

    test_subtraction<builtin_decimal32_t>();
    test_subtraction<builtin_decimal64_t>();
    test_subtraction<builtin_decimal128_t>();

    test_multiplication<builtin_decimal32_t>();
    test_multiplication<builtin_decimal64_t>();
    test_multiplication<builtin_decimal128_t>();

    test_division<builtin_decimal32_t>();
    test_division<builtin_decimal64_t>();
    test_division<builtin_decimal128_t>();

    test_increment_decrement<builtin_decimal32_t>();
    test_increment_decrement<builtin_decimal64_t>();
    test_increment_decrement<builtin_decimal128_t>();

    test_limits<builtin_decimal32_t>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
