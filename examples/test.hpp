// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Some trivial testing facilities so that the library examples can run without boost
// Running the full test suite in the test/ directory does require boost

#ifndef BOOST_DECIMAL_EXAMPLES_TEST_HPP
#define BOOST_DECIMAL_EXAMPLES_TEST_HPP

#include <boost/decimal/iostream.hpp>
#include <iostream>
#include <type_traits>
#include <limits>

namespace boost {
namespace decimal {
namespace test {

static int errors = 0;

inline int report_errors()
{
    // Limited on the upper bound of main return on some platforms
    return errors > 255 ? 255 : errors;
}

inline void test(const bool x)
{
    if (!x)
    {
        ++errors;
        std::cerr << "Test Failed at line: " << __LINE__ << std::endl;
    }
}

template <typename T, std::enable_if_t<!std::is_floating_point<T>::value, bool> = true>
void test_eq(const T lhs, const T rhs)
{
    if (lhs != rhs)
    {
        ++errors;
        std::cerr << "Failed equality test for: " << lhs << " and " << rhs << " at line: " << __LINE__ << std::endl;
    }
}

template <typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
void test_eq(const T a, const T b)
{
    // Knuth's approximate float equality from The Art of Computer Programming
    // See also: https://stackoverflow.com/questions/17333/how-do-you-compare-float-and-double-while-accounting-for-precision-loss
    using std::fabs;
    test(fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<T>::epsilon()));
}

} // namespace test
} // namespace decimal
} // namespace boost

#define BOOST_DECIMAL_TEST(x) boost::decimal::test::test(x);
#define BOOST_DECIMAL_TEST_EQ(lhs, rhs) boost::decimal::test::test_eq(lhs, rhs)

#endif // BOOST_DECIMAL_EXAMPLES_TEST_HPP
