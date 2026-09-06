// Copyright 2023 - 2026 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "testing_config.hpp"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>

#include <boost/decimal.hpp>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <boost/core/lightweight_test.hpp>

auto my_zero() -> boost::decimal::decimal32_t&;
auto my_one () -> boost::decimal::decimal32_t&;

namespace local
{
  template<typename IntegralTimePointType,
           typename ClockType = std::chrono::high_resolution_clock>
  auto time_point() noexcept -> IntegralTimePointType
  {
    using local_integral_time_point_type = IntegralTimePointType;
    using local_clock_type               = ClockType;

    const auto current_now =
      static_cast<std::uintmax_t>
      (
        std::chrono::duration_cast<std::chrono::nanoseconds>
        (
          local_clock_type::now().time_since_epoch()
        ).count()
      );

    return static_cast<local_integral_time_point_type>(current_now);
  }

  template<typename NumericType>
  auto is_close_fraction(const NumericType& a,
                         const NumericType& b,
                         const NumericType& tol) noexcept -> bool
  {
    using std::fabs;

    auto result_is_ok = bool { };

    if(b == static_cast<NumericType>(0))
    {
      result_is_ok = (fabs(a - b) < tol); // LCOV_EXCL_LINE
    }
    else
    {
      const auto delta = fabs(1 - (a / b));

      result_is_ok = (delta < tol);
    }

    return result_is_ok;
  }

  auto test_acosh(const std::int32_t tol_factor, const long double range_lo, const long double range_hi) -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    // Use a pseudo-random seed value.
    std::mt19937_64 gen(std::random_device{}());

    gen.seed(time_point<typename std::mt19937_64::result_type>());

    auto dis =
      std::uniform_real_distribution<float>
      {
        static_cast<float>(range_lo),
        static_cast<float>(range_hi)
      };

    auto result_is_ok = true;

    auto trials = static_cast<std::uint32_t>(UINT8_C(0));

    #if !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH)
    constexpr auto count = UINT32_C(0x400);
    #else
    constexpr auto count = UINT32_C(0x40);
    #endif

    for( ; trials < count; ++trials)
    {
      const auto x_flt = dis(gen);
      const auto x_dec = static_cast<decimal_type>(x_flt);

      using std::acosh;

      const auto val_flt = acosh(x_flt);
      const auto val_dec = acosh(x_dec);

      const auto result_val_is_ok = is_close_fraction(val_flt, static_cast<float>(val_dec), std::numeric_limits<float>::epsilon() * static_cast<float>(tol_factor));

      result_is_ok = (result_val_is_ok && result_is_ok);

      if(!result_val_is_ok)
      {
          // LCOV_EXCL_START
        std::cout << "x_flt  : " <<                    x_flt   << std::endl;
        std::cout << "val_flt: " << std::scientific << val_flt << std::endl;
        std::cout << "val_dec: " << std::scientific << val_dec << std::endl;

        break;
          // LCOV_EXCL_STOP
      }
    }

    BOOST_TEST(result_is_ok);

    return result_is_ok;
  }

  auto test_acosh_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    std::uniform_real_distribution<float> dist(1.01F, 1.04F);

    auto result_is_ok = true;

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_nan_pos = acosh(std::numeric_limits<decimal_type>::quiet_NaN() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_nan_pos_is_ok = isnan(val_nan_pos) && (!signbit(val_nan_pos));

      BOOST_TEST(result_val_nan_pos_is_ok);

      result_is_ok = (result_val_nan_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_nan_neg = acosh(-std::numeric_limits<decimal_type>::quiet_NaN() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_nan_neg_is_ok = isnan(val_nan_neg) && (signbit(val_nan_neg));

      BOOST_TEST(result_val_nan_neg_is_ok);

      result_is_ok = (result_val_nan_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_pos = acosh(std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_pos_is_ok = (isinf(val_inf_pos) && (!signbit(val_inf_pos)));

      BOOST_TEST(result_val_inf_pos_is_ok);

      result_is_ok = (result_val_inf_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_one = acosh(::my_one());

      const auto result_val_one_is_ok = (val_one == ::my_zero());

      BOOST_TEST(result_val_one_is_ok);

      result_is_ok = (result_val_one_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_lt_one = acosh(::my_one() / static_cast<decimal_type>(dist(gen) * (dist(gen) * dist(gen))));

      const auto result_val_lt_one_is_ok = (isnan(val_lt_one) && signbit(val_lt_one));

      BOOST_TEST(result_val_lt_one_is_ok);

      result_is_ok = (result_val_lt_one_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_pos = acosh(::my_zero());

      const auto result_val_zero_pos_is_ok = (isnan(val_zero_pos) && (signbit(val_zero_pos)));

      BOOST_TEST(result_val_zero_pos_is_ok);

      result_is_ok = (result_val_zero_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_neg = acosh(-::my_zero());

      const auto result_val_zero_neg_is_ok = (isnan(val_zero_neg) && (signbit(val_zero_neg)));

      BOOST_TEST(result_val_zero_neg_is_ok);

      result_is_ok = (result_val_zero_neg_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  // These control values cover the range where this function gives an argument
  // to log1p that is larger than its own argument. The random tests use
  // decimal32_t and compare with float. Thus they cannot find an error that is
  // smaller than the epsilon of float.
  template<typename DecimalType>
  auto test_acosh_ctrl(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 28U>;

    const str_ctrl_array_type x_strings =
    {{
      "1.000000000000000001", "1.000000000000000002", "1.000000000000000005", "1.000000000000000008",
      "1.000000001", "1.000000002", "1.000000005", "1.000000008",
      "1.000001", "1.000002", "1.000005", "1.000008",
      "1.00001", "1.00002", "1.00005", "1.00008",
      "1.0001", "1.0002", "1.0005", "1.0008",
      "1.005", "1.01",  "1.02",  "1.03",
      "1.05",  "1.08",  "1.1",   "1.2",
    }};

    const str_ctrl_array_type ctrl_strings =
    {{
      // N[ArcCosh[x], 36], <each string in x_strings>]
      "1.41421356237309504868383759401194016E-9",
      "1.99999999999999999966666666666666667E-9",
      "3.16227766016837933068127785269589381E-9",
      "3.99999999999999999733333333333333334E-9",
      "0.0000447213595462690139665223493719230952",
      "0.0000632455531928266611108266896027538323",
      "0.0000999999999583333333802083332635788692",
      "0.000126491106322407769160588286917350628",
      "0.00141421344452199136754017067219889749",
      "0.00199999966666681666657738101314479653",
      "0.00316227634255416991062154971792914949",
      "0.00399999733333813332190479301578137979",
      "0.00447213222822800212312844663302815991",
      "0.00632454477945865865195824130004375741",
      "0.00999995833380207635800558204765207609",
      "0.0126490263147871033087618841493129510",
      "0.0141420177752523242440634748427880047",
      "0.0199996666816657738702832243279075920",
      "0.0316214591342017672695329055568156238",
      "0.0399973338132190787209979359571195932",
      "0.0999583801386973304627899242727135732",
      "0.141303769485648577351151646974354648",
      "0.199668157798415126654606249409538887",
      "0.244340698822827497531200999061269201",
      "0.314924756603847871743403417928208223",
      "0.397380220698482812949273831900413936",
      "0.443568254385115189132911066352498087",
      "0.62236250371477866780685115857913059",
    }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(x_strings[i], x_strings[i] + std::strlen(x_strings[i]), x_arg));
      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      constexpr decimal_type one { 1, 0 };

      const bool result_acosh_is_ok
      {
        (fpclassify(x_arg - one) == FP_ZERO) ? (fpclassify(acosh(x_arg)) == FP_ZERO)
                                             : is_close_fraction(acosh(x_arg), ctrl_value, my_tol)
      };

      result_is_ok = (result_acosh_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  template<typename DecimalType>
  auto test_acosh_ranges(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 21U>;

    const str_ctrl_array_type ctrl_strings =
    {{
      // Table[N[ArcCosh[(100 + n + 1)/10], 36], {n, 0, 20, 1}]
      "3.00322280571016789643016934137543854",
      "3.01312327130394315089630791822982970",
      "3.02292621256599303559916369156870657",
      "3.03263354097180014693165540204685073",
      "3.04224711209332853140618699490135328",
      "3.05176872776731232647948239055861008",
      "3.06120013815899735261125176989627186",
      "3.07054304372735053682959608126111137",
      "3.07979909709735066642123374867270193",
      "3.08896990484460301791647985373629509",
      "3.09805702919717730217389866230122292",
      "3.10706198965925077333209213029557198",
      "3.11598626456084414661162876161931402",
      "3.12483129253766523107456633675830233",
      "3.13359847394482215732811372516096699",
      "3.14228917220793317341287173492363033",
      "3.15090471511494174360045898295530853",
      "3.15944639605174279175130758084948281",
      "3.16791547518453717640586010775811933",
      "3.17631318059165576688921941338737732",
      "3.18464070934743080126507223242363621"
  }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { 100U + i + 1U, -1 };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      const decimal_type value { acosh(x_arg) };

      const auto result_asinh_is_ok = is_close_fraction(value, ctrl_value, my_tol);

      result_is_ok = (result_asinh_is_ok && result_is_ok);
    }

    return result_is_ok;
  }
} // namespace local

auto main() -> int
{
  auto result_is_ok = true;

  const auto result_small_is_ok  = local::test_acosh(INT32_C(32), 1.2L, 1.60L);
  const auto result_medium_is_ok = local::test_acosh(INT32_C(16), 1.59L, 10.1L);
  const auto result_large_is_ok  = local::test_acosh(INT32_C(16), 1.0E+01L, 1.0E+26L);

  BOOST_TEST(result_small_is_ok);
  BOOST_TEST(result_medium_is_ok);
  BOOST_TEST(result_large_is_ok);

  const auto result_edge_is_ok = local::test_acosh_edge();

  const auto result_ranges_is_ok = (result_small_is_ok && result_medium_is_ok && result_large_is_ok);

  result_is_ok = (result_ranges_is_ok && result_is_ok);

  BOOST_TEST(result_edge_is_ok);

  result_is_ok = (result_edge_is_ok && result_is_ok);

  {
    using namespace boost::decimal;

    const bool result_acosh_dec32_t_is_ok       = local::test_acosh_ctrl<decimal32_t>      (16);
    const bool result_acosh_dec64_t_is_ok       = local::test_acosh_ctrl<decimal64_t>      (16);
    const bool result_acosh_dec128_t_is_ok      = local::test_acosh_ctrl<decimal128_t>     (16);
    const bool result_acosh_dec_fast32_t_is_ok  = local::test_acosh_ctrl<decimal_fast32_t> (16);
    const bool result_acosh_dec_fast64_t_is_ok  = local::test_acosh_ctrl<decimal_fast64_t> (16);
    const bool result_acosh_dec_fast128_t_is_ok = local::test_acosh_ctrl<decimal_fast128_t>(16);

    BOOST_TEST(result_acosh_dec32_t_is_ok);
    BOOST_TEST(result_acosh_dec64_t_is_ok);
    BOOST_TEST(result_acosh_dec128_t_is_ok);
    BOOST_TEST(result_acosh_dec_fast32_t_is_ok);
    BOOST_TEST(result_acosh_dec_fast64_t_is_ok);
    BOOST_TEST(result_acosh_dec_fast128_t_is_ok);

    const auto result_ctrl_is_ok =
    (
         result_acosh_dec32_t_is_ok
      && result_acosh_dec64_t_is_ok
      && result_acosh_dec128_t_is_ok
      && result_acosh_dec_fast32_t_is_ok
      && result_acosh_dec_fast64_t_is_ok
      && result_acosh_dec_fast128_t_is_ok
    );

    BOOST_TEST(result_ctrl_is_ok);

    result_is_ok = (result_ctrl_is_ok && result_is_ok);
  }

  {
    using namespace boost::decimal;

    const bool result_acosh_ranges_dec32_t_is_ok       = local::test_acosh_ranges<decimal32_t>      (16);
    const bool result_acosh_ranges_dec64_t_is_ok       = local::test_acosh_ranges<decimal64_t>      (16);
    const bool result_acosh_ranges_dec128_t_is_ok      = local::test_acosh_ranges<decimal128_t>     (16);
    const bool result_acosh_ranges_dec_fast32_t_is_ok  = local::test_acosh_ranges<decimal_fast32_t> (16);
    const bool result_acosh_ranges_dec_fast64_t_is_ok  = local::test_acosh_ranges<decimal_fast64_t> (16);
    const bool result_acosh_ranges_dec_fast128_t_is_ok = local::test_acosh_ranges<decimal_fast128_t>(16);

    BOOST_TEST(result_acosh_ranges_dec32_t_is_ok);
    BOOST_TEST(result_acosh_ranges_dec64_t_is_ok);
    BOOST_TEST(result_acosh_ranges_dec128_t_is_ok);
    BOOST_TEST(result_acosh_ranges_dec_fast32_t_is_ok);
    BOOST_TEST(result_acosh_ranges_dec_fast64_t_is_ok);
    BOOST_TEST(result_acosh_ranges_dec_fast128_t_is_ok);
  }

  BOOST_TEST(result_is_ok);

  return boost::report_errors();
}

auto my_zero() -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_zero { 0, 0 }; return val_zero; }
auto my_one () -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_one  { 1, 0 }; return val_one; }
