// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <array>

using namespace boost::decimal;

template <typename T>
void test_signaling()
{
    const std::array<const char*, 6> lhs_values { "nan(snan)", "SNAN", "SNAN(42)", "SNAN42", "sNaN400JUNK", "nan(snan42)" };
    const std::array<const char*, 6> rhs_values { "nan(snan)", "SNAN", "SNAN(43)", "SNAN43", "SnAn410JUNK", "nan(snan4000)" };

    for (std::size_t i {}; i < lhs_values.size(); ++i)
    {
        std::cerr << "I: " << i << std::endl;
        const T lhs {lhs_values[i]};
        const T rhs {rhs_values[i]};

        BOOST_TEST(isnan(lhs));
        BOOST_TEST(isnan(rhs));

        BOOST_TEST(issignaling(lhs));
        BOOST_TEST(issignaling(rhs));

        if (i >= 2)
        {
            // A nan with a higher payload compares higher
            BOOST_TEST(comparetotal(lhs, rhs));
        }
    }
}

int main()
{
    test_signaling<decimal32_t>();

    return boost::report_errors();
}
