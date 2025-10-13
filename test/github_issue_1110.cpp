// Copyright 2025 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1110

#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/cmath.hpp>
#include <boost/decimal/iostream.hpp>

#include <boost/core/lightweight_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace local {

auto test_near1() -> void;
auto test_large() -> void;

auto test_near1() -> void
{
  const boost::decimal::decimal128_t one { 1 };
  const boost::decimal::decimal128_t del { 1, -32 };
  const boost::decimal::decimal128_t sum { one + del };

  const boost::decimal::decimal128_t sqr { sqrt(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10) << sqr;

    BOOST_TEST(strm.str() == "1.000000000000000000000000000000005");
  }

  const boost::decimal::decimal128_t cbr { cbrt(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10)<< cbr;

    BOOST_TEST(strm.str() == "1.000000000000000000000000000000003");
  }

  const boost::decimal::decimal128_t lgt { log10(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10)<< lgt;

    BOOST_TEST(strm.str() == "4.4e-33");
  }
}

auto test_large() -> void
{
  const boost::decimal::decimal128_t one { 1, 100 };
  const boost::decimal::decimal128_t del { 1,  68 };
  const boost::decimal::decimal128_t sum { one + del };

  const boost::decimal::decimal128_t sqr { sqrt(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10) << sqr;

    BOOST_TEST(strm.str() == "1.000000000000000000000000000000005e+50");
  }

  const boost::decimal::decimal128_t cbr { cbrt(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10)<< cbr;

    BOOST_TEST(strm.str() == "2154434690031883721759293566519356");
  }

  const boost::decimal::decimal128_t lgt { log10(sum) };

  {
    std::stringstream strm { };

    strm << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::digits10)<< lgt;

    BOOST_TEST(strm.str() == "100");
  }
}

} // namespace local

auto main() -> int
{
  local::test_near1();
  local::test_large();

  return boost::report_errors();
}
