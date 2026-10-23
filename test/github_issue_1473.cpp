// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// Each result is subnormal. The expected values are the exact results rounded once
// to nearest even. The old code rounded them to the precision first, then again.
template <typename T>
void check_div(const T x, const T y, const T expected) { BOOST_TEST_EQ(x / y, expected); }

template <typename T>
void check_mul(const T x, const T y, const T expected) { BOOST_TEST_EQ(x * y, expected); }

int main()
{
    check_div(2517092e-100_DF, 3317964e-5_DF, 758625e-101_DF);
    check_div(9919733e-99_DF, 9870042e-3_DF, 100503e-101_DF);
    check_div(5821354e-98_DF, 9794776e-3_DF, 594333e-101_DF);

    check_div(3991108143056727e-397_DD, 8953368188687686e-14_DD, 445766113818415e-398_DD);
    check_div(1701484450986967e-397_DD, 5488491342030049e-14_DD, 310009498959623e-398_DD);
    check_div(2060890574973428e-395_DD, 9757842586175888e-12_DD, 211203507001961e-398_DD);

    check_mul(9805768160164496e-398_DD, 6063043991034273e-17_DD, 594528037209605e-398_DD);
    check_mul(5777325332995362e-398_DD, 1360569059678292e-16_DD, 786045009576907e-398_DD);
    check_mul(1896491582107219e-396_DD, 1464181667452488e-18_DD, 277680820699935e-398_DD);

    check_div(2202560125678026287889671166873600e-6175_DL, 9829072753674208315303658856773732e-31_DL,
              22408625725704255247582439061311e-6176_DL);
    check_div(9801164418570411013518952357786263e-6174_DL, 7078776746611438503682924523953845e-30_DL,
              138458447969307135866449039302757e-6176_DL);
    check_div(2963408436469547542125654596857873e-6175_DL, 6489181997501819343732049550902432e-32_DL,
              456669028178033729773136591928213e-6176_DL);

    return boost::report_errors();
}
