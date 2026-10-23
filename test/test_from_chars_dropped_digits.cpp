// Copyright 2026 Pavel P
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <system_error>

using namespace boost::decimal;

template <typename T>
static T parse(const char* str)
{
    T value {};
    const auto r = from_chars(str, str + std::strlen(str), value);
    BOOST_TEST(r.ec == std::errc());
    return value;
}

// More digits than the parser keeps: the dropped ones still decide the rounding
template <typename T>
static void test(const char* str, const char* rounded)
{
    BOOST_TEST_EQ(parse<T>(str), parse<T>(rounded));
}

template <typename T>
static void test_32()
{
    // The 19 digits kept are an exact tie; the dropped 1 puts the value above it
    test<T>("1.00000050000000000000000001", "1.000001");
    test<T>("100000050000000000000000001", "1000001e20");
    test<T>("1.00000050000000000000000001e5", "100000.1");
    test<T>("-1.00000050000000000000000001", "-1.000001");
    // Above half the smallest subnormal rounds up to it, not down to zero
    test<T>(".50000000000000000000000001e-101", "1e-101");
    // Only zeros dropped: still a tie, rounded to even
    test<T>("1.00000050000000000000000000", "1.000000");
}

template <typename T>
static void test_64()
{
    test<T>("1.0000000000000005000000001", "1.000000000000001");
    test<T>("1.0000000000000005000000000", "1.000000000000000");
}

template <typename T>
static void test_128()
{
    // 34 digits of precision, 38 kept
    test<T>("1.000000000000000000000000000000000500001", "1.000000000000000000000000000000001");
    test<T>("1.000000000000000000000000000000000500000", "1.000000000000000000000000000000000");
}

int main()
{
    test_32<decimal32_t>();
    test_32<decimal_fast32_t>();
    test_64<decimal64_t>();
    test_64<decimal_fast64_t>();
    test_128<decimal128_t>();
    test_128<decimal_fast128_t>();

    return boost::report_errors();
}
