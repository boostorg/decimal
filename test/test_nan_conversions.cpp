// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: IEEE 754-2008 Section 7.2

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <functional>
#include <array>
#include <random>

static std::mt19937_64 rng{42};
static std::uniform_int_distribution<unsigned> dist{5, 100};

using namespace boost::decimal;

template <typename T, typename Func, typename U>
void test(const T lhs, const T rhs, Func op, U payload)
{
    BOOST_TEST(isnan(lhs) || isnan(rhs));
    BOOST_TEST(issignaling(lhs) || issignaling(rhs));

    const auto res {op(lhs, rhs)};

    BOOST_TEST(isnan(res));
    BOOST_TEST(!issignaling(res));

    if (payload > 0U)
    {
        const auto result_payload {read_payload(res)};
        BOOST_TEST_EQ(result_payload, payload);
    }
}

template <typename T, typename Func, typename U>
void test_qnan_preservation(const T lhs, const T rhs, Func op, U payload)
{
    BOOST_TEST(isnan(lhs) || isnan(rhs));

    const auto res {op(lhs, rhs)};

    BOOST_TEST(isnan(res));
    BOOST_TEST(!issignaling(res));

    if (payload > 0U)
    {
        const auto result_payload {read_payload(res)};
        BOOST_TEST_EQ(result_payload, payload);
    }
}

template <typename T>
void generate_tests()
{
    constexpr std::size_t N {5};

    const std::array<unsigned, N> payloads {0, 0, 1, 2, 3};
    const std::array<const char*, N> nans {"sNaN", "SNAN", "snan1", "SnAn2", "SNAN3"};

    for (std::size_t i = 0; i < N; ++i)
    {
        const T value1 {nans[i]};
        const T value2 {dist(rng)};
        const auto current_payload {payloads[i]};

        test(value1, value2, std::plus<>(), current_payload);
        test(value1, value2, std::minus<>(), current_payload);
        test(value1, value2, std::multiplies<>(), current_payload);
        test(value1, value2, std::divides<>(), current_payload);
        test(value1, value2, std::modulus<>(), current_payload);

        test(value2, value1, std::plus<>(), current_payload);
        test(value2, value1, std::minus<>(), current_payload);
        test(value2, value1, std::multiplies<>(), current_payload);
        test(value2, value1, std::divides<>(), current_payload);
        test(value2, value1, std::modulus<>(), current_payload);
    }

}

template <typename T>
void generate_qnan_tests()
{
    constexpr std::size_t N {5};

    const std::array<unsigned, N> payloads {0, 0, 1, 2, 3};
    const std::array<const char*, N> nans {"NaN", "NAN", "nan1", "nAn2", "NAN(3)"};

    for (std::size_t i = 0; i < N; ++i)
    {
        const T value1 {nans[i]};
        const T value2 {dist(rng)};
        const auto current_payload {payloads[i]};

        test_qnan_preservation(value1, value2, std::plus<>(), current_payload);
        test_qnan_preservation(value1, value2, std::minus<>(), current_payload);
        test_qnan_preservation(value1, value2, std::multiplies<>(), current_payload);
        test_qnan_preservation(value1, value2, std::divides<>(), current_payload);
        test_qnan_preservation(value1, value2, std::modulus<>(), current_payload);

        test_qnan_preservation(value2, value1, std::plus<>(), current_payload);
        test_qnan_preservation(value2, value1, std::minus<>(), current_payload);
        test_qnan_preservation(value2, value1, std::multiplies<>(), current_payload);
        test_qnan_preservation(value2, value1, std::divides<>(), current_payload);
        test_qnan_preservation(value2, value1, std::modulus<>(), current_payload);
    }

}

int main()
{
    generate_tests<decimal32_t>();
    generate_tests<decimal64_t>();
    generate_tests<decimal128_t>();

    generate_tests<decimal_fast32_t>();
    generate_tests<decimal_fast64_t>();
    generate_tests<decimal_fast128_t>();

    generate_qnan_tests<decimal32_t>();
    generate_qnan_tests<decimal64_t>();
    generate_qnan_tests<decimal128_t>();

    generate_qnan_tests<decimal_fast32_t>();
    generate_qnan_tests<decimal_fast64_t>();
    generate_qnan_tests<decimal_fast128_t>();

    return boost::report_errors();
}
