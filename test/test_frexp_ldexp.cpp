// Copyright 2023 - 2026 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "testing_config.hpp"
#include <chrono>
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

template<typename DecimalType> auto my_zero() -> DecimalType&;
template<typename DecimalType> auto my_one () -> DecimalType&;
template<typename DecimalType> auto my_inf () -> DecimalType&;
template<typename DecimalType> auto my_nan () -> DecimalType&;

namespace local
{
  template<typename IntegralTimePointType,
           typename ClockType = std::chrono::high_resolution_clock>
  auto time_point() -> IntegralTimePointType
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
                         const NumericType& tol) -> bool
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

  typedef struct test_frexp_ldexp_ctrl
  {
    float         float_value_lo;
    float         float_value_hi;
    bool          b_neg;
    std::uint32_t count;
  }
  test_frexp_ldexp_ctrl;

  auto test_frexp_ldexp_impl(const test_frexp_ldexp_ctrl& ctrl, const int tol_factor) -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::random_device rd;
    std::mt19937_64 gen(rd());

    gen.seed(time_point<typename std::mt19937_64::result_type>());

    std::uniform_real_distribution<float> dis(ctrl.float_value_lo, ctrl.float_value_hi);

    auto result_is_ok = true;

    auto trials = static_cast<std::uint32_t>(UINT8_C(0));

    for( ; trials < ctrl.count; ++trials)
    {
      auto flt_start = float { };

      do
      {
        flt_start = dis(gen);
      }
      while (flt_start == static_cast<float>(0.0L));

      if(ctrl.b_neg) { flt_start = -flt_start; }

      const auto dec = static_cast<decimal_type>(flt_start);
      const auto flt = static_cast<float>(dec);

      using std::frexp;

      int n_flt;
      int n_dec;
      const auto frexp_flt = frexp(flt, &n_flt);
      const auto frexp_dec = frexp(dec, &n_dec);

      using std::ldexp;

      const auto ldexp_flt = ldexp(frexp_flt, n_flt);
      const auto ldexp_dec = ldexp(frexp_dec, n_dec);

      const auto ldexp_dec_as_float = static_cast<float>(ldexp_dec);

      const auto result_frexp_ldexp_is_ok = is_close_fraction(ldexp_flt, ldexp_dec_as_float, static_cast<float>(std::numeric_limits<decimal_type>::epsilon()) * static_cast<float>(tol_factor));

      if(!result_frexp_ldexp_is_ok)
      {
          // LCOV_EXCL_START
        std::cout << "flt      : " << std::scientific << std::setprecision(std::numeric_limits<float>::digits10) << flt       << std::endl;
        std::cout << "frexp_flt: " << std::scientific << std::setprecision(std::numeric_limits<float>::digits10) << frexp_flt << std::endl;
        std::cout << "frexp_dec: " << std::scientific << std::setprecision(std::numeric_limits<float>::digits10) << frexp_dec << std::endl;
        std::cout << "ldexp_flt: " << std::scientific << std::setprecision(std::numeric_limits<float>::digits10) << ldexp_flt << std::endl;
        std::cout << "ldexp_dec: " << std::scientific << std::setprecision(std::numeric_limits<float>::digits10) << ldexp_dec << std::endl;

        break;
          // LCOV_EXCL_STOP
      }

      result_is_ok = (result_frexp_ldexp_is_ok && result_is_ok);
    }

    result_is_ok = ((trials == ctrl.count) && result_is_ok);

    return result_is_ok;
  }

  auto test_frexp_ldexp() -> bool
  {
    #if !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH)
    constexpr auto test_frexp_ldexp_depth = UINT32_C(0x800);
    #else
    constexpr auto test_frexp_ldexp_depth = UINT32_C(0x80);
    #endif

    const local::test_frexp_ldexp_ctrl flt_ctrl[static_cast<std::size_t>(UINT8_C(7))] =
    {
      { 8388606.5F, 8388607.5F, false, test_frexp_ldexp_depth },
      { -1.0E7F   , +1.0E7F,    false, test_frexp_ldexp_depth },
      { +1.0E-20F , +1.0E-1F,   false, test_frexp_ldexp_depth },
      { +1.0E-20F , +1.0E-1F,   true,  test_frexp_ldexp_depth },
      { +1.0E-28F , +1.0E-26F,  false, test_frexp_ldexp_depth },
      { +10.0F    , +1.0E12F,   false, test_frexp_ldexp_depth },
      { +10.0F    , +1.0E12F,   true , test_frexp_ldexp_depth },
    };

    auto result_is_ok = true;

    for(const auto& ctrl : flt_ctrl)
    {
      const auto result_test_frexp_ldexp_is_ok = local::test_frexp_ldexp_impl(ctrl, 16);

      BOOST_TEST(result_test_frexp_ldexp_is_ok);

      result_is_ok = (result_test_frexp_ldexp_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  template<typename FloatingPointType>
  auto test_frexp_ldexp_exact_impl(double f_in, double fr_ctrl, int nr_ctrl) -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    using local_float_type = FloatingPointType;

    const auto dec = static_cast<decimal_type>(static_cast<local_float_type>(f_in));

    int n_dec;
    const auto frexp_dec = frexp(dec, &n_dec);

    const auto result_frexp_is_ok =
      (
           (frexp_dec == static_cast<decimal_type>(static_cast<local_float_type>(fr_ctrl)))
        && (n_dec == nr_ctrl)
      );

    auto result_is_ok = result_frexp_is_ok;

    const auto ldexp_dec = ldexp(frexp_dec, n_dec);

    const auto result_ldexp_is_ok = (ldexp_dec == static_cast<decimal_type>(static_cast<local_float_type>(f_in)));

    result_is_ok = (result_ldexp_is_ok && result_is_ok);

    BOOST_TEST(result_is_ok);

    return result_is_ok;
  }

  auto test_frexp_ldexp_exact() -> bool
  {
    // 7.625L, 0.953125L, 3
    auto result_frexp_ldexp_exact_is_ok = true;

    result_frexp_ldexp_exact_is_ok = (test_frexp_ldexp_exact_impl<float>(+7.625, +0.953125,  3) && result_frexp_ldexp_exact_is_ok);
    result_frexp_ldexp_exact_is_ok = (test_frexp_ldexp_exact_impl<float>(+0.125, +0.5,      -2) && result_frexp_ldexp_exact_is_ok);
    result_frexp_ldexp_exact_is_ok = (test_frexp_ldexp_exact_impl<float>(-0.125, -0.5,      -2) && result_frexp_ldexp_exact_is_ok);

    return result_frexp_ldexp_exact_is_ok;
  }

  auto test_frexp_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    gen.seed(time_point<typename std::mt19937_64::result_type>());

    std::uniform_real_distribution<float>
      dist
      (
        static_cast<float>(1.01L),
        static_cast<float>(1.04L)
      );

    constexpr decimal_type zero {0};

    auto n_dec = int { };
    auto frexp_dec = zero;

    auto result_is_ok = true;

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type arg_zero { ::my_zero<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      frexp_dec = frexp(arg_zero, &n_dec);

      const auto result_zero_is_ok = ((frexp_dec == 0) && (n_dec == 0));

      result_is_ok = (result_zero_is_ok && result_is_ok);

      BOOST_TEST(result_is_ok);
    }

    // frexp keeps the sign of a zero.
    frexp_dec = frexp(-zero, &n_dec);
    BOOST_TEST_EQ(frexp_dec, zero);
    BOOST_TEST(signbit(frexp_dec));
    BOOST_TEST_EQ(n_dec, 0);

    // The narrow pass of decimal64 cannot decide these fractions, and the wide pass rounds them.
    {
      using namespace boost::decimal::literals;

      auto n_dd = int { };
      BOOST_TEST_EQ(frexp(262e200_DD, &n_dd), 0.6685196997600160_DD);
      BOOST_TEST_EQ(n_dd, 673);
      BOOST_TEST_EQ(frexp(110610e-22_DD, &n_dd), 0.7970290476535209_DD);
      BOOST_TEST_EQ(n_dd, -56);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type arg_inf { ::my_inf<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      frexp_dec = frexp(arg_inf, &n_dec);

      const auto result_inf_is_ok = (isinf(frexp_dec) && (n_dec == 0));
      result_is_ok = (result_inf_is_ok && result_is_ok);
      BOOST_TEST(result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type arg_nan { ::my_nan<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      frexp_dec = frexp(arg_nan, &n_dec);

      const auto result_nan_is_ok = (isnan(frexp_dec) && (n_dec == 0));

      result_is_ok = (result_nan_is_ok && result_is_ok);

      BOOST_TEST(result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type arg_inf { ::my_inf<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      int n_dummy { };
      const auto frexp_inf = frexp(arg_inf, &n_dummy);

      const volatile auto result_frexp_inf_is_ok = isinf(frexp_inf);

      BOOST_TEST(result_frexp_inf_is_ok);

      result_is_ok = (result_frexp_inf_is_ok && result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      decimal_type
        arg_inf
        {
            (::my_one<decimal_type>() * static_cast<decimal_type>(dist(gen)))
          / (::my_zero<decimal_type>() * static_cast<decimal_type>(dist(gen)))
        };

      int n_dummy { };
      const auto frexp_inf = frexp(arg_inf, &n_dummy);

      const volatile auto result_frexp_inf_is_ok = isinf(frexp_inf);

      BOOST_TEST(result_frexp_inf_is_ok);

      result_is_ok = (result_frexp_inf_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  auto test_ldexp_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    gen.seed(time_point<typename std::mt19937_64::result_type>());

    std::uniform_real_distribution<float>
      dist
      (
        static_cast<float>(1.01L),
        static_cast<float>(1.04L)
      );

    auto result_is_ok = true;

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const auto arg_zero { ::my_zero<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      auto ldexp_dec = ldexp(arg_zero, 0);
      auto result_zero_is_ok = (ldexp_dec == 0);

      ldexp_dec = ldexp(arg_zero, 3);
      result_zero_is_ok = ((ldexp_dec == 0) && result_zero_is_ok);

      result_is_ok = (result_zero_is_ok && result_is_ok);
      BOOST_TEST(result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const auto arg_inf { ::my_inf<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      auto ldexp_dec = ldexp(arg_inf, 0);
      auto result_inf_is_ok = isinf(ldexp_dec);

      ldexp_dec = ldexp(arg_inf, 3);
      result_inf_is_ok = (isinf(ldexp_dec) && result_inf_is_ok);

      result_is_ok = (result_inf_is_ok && result_is_ok);
      BOOST_TEST(result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const auto arg_nan { ::my_nan<decimal_type>() * static_cast<decimal_type>(dist(gen)) };

      auto ldexp_dec = ldexp(arg_nan, 0);
      auto result_nan_is_ok = isnan(ldexp_dec);

      ldexp_dec = ldexp(arg_nan, 3);
      result_nan_is_ok = (isnan(ldexp_dec) && result_nan_is_ok);

      result_is_ok = (result_nan_is_ok && result_is_ok);
      BOOST_TEST(result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type
        arg_inf
        {
            (::my_one<decimal_type>() * static_cast<decimal_type>(dist(gen)))
          / (::my_zero<decimal_type>() * static_cast<decimal_type>(dist(gen)))
        };

      const auto ldexp_inf = ldexp(arg_inf, 3);

      const volatile auto result_ldexp_inf_is_ok = isinf(ldexp_inf);

      BOOST_TEST(result_ldexp_inf_is_ok);

      result_is_ok = (result_ldexp_inf_is_ok && result_is_ok);
    }

    for(auto index = static_cast<unsigned>(UINT8_C(0)); index < static_cast<unsigned>(UINT8_C(4)); ++index)
    {
      static_cast<void>(index);

      const decimal_type arg_nan { sqrt(-::my_one<decimal_type>()) * static_cast<decimal_type>(dist(gen)) };

      const auto ldexp_nan = ldexp(arg_nan, 3);

      const volatile auto result_ldexp_nan_is_ok = isnan(ldexp_nan);

      BOOST_TEST(result_ldexp_nan_is_ok);

      result_is_ok = (result_ldexp_nan_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  auto test_ldexp_exact() -> bool
  {
    using namespace boost::decimal::literals;
    using boost::decimal::decimal32_t;

    const auto errors_before = boost::detail::test_errors();

    // ldexp rounds v * 2^e once to p digits. The values come from exact rational arithmetic.
    BOOST_TEST(ldexp(6.973172_DF, -23) == 8.312669e-7_DF);
    BOOST_TEST(ldexp(9.633772e12_DF, -37) == 70.09492_DF);
    BOOST_TEST(ldexp(8.748035862046145e9_DD, -43) == 9.945365334313175e-4_DD);
    BOOST_TEST(ldexp(3.181742449603263e18_DD, -53) == 353.2443725976789_DD);

    // r has one digit more than the estimate from log2(v) gives, and the rounding drops it.
    BOOST_TEST(ldexp(5.614342678885985_DD, 54) == 1.011390063862448e17_DD);
    BOOST_TEST(ldexp(410175947268288019432414138148683e172_DL, 978) == 1.047866482636335195999053662046774e499_DL);

    // The result is in range although 2^e is not.
    BOOST_TEST(ldexp(4.4e-77_DF, 375) == 3.386110e36_DF);
    BOOST_TEST(ldexp(9.5e90_DF, -600) == 2.289424e-90_DF);
    BOOST_TEST(ldexp(1e-300_DD, 1500) == 3.507466211043404e151_DD);
    BOOST_TEST(ldexp(3e300_DD, -2000) == 2.612942944865165e-302_DD);
    BOOST_TEST(ldexp(1e-6000_DL, 30000) == 7.940903519132960324132517843492703e3030_DL);
    BOOST_TEST(ldexp(7e6000_DL, -40000) == 4.418465626727089245621757459628134e-6041_DL);

    // Small cases are exact. If the result needs more than p digits, or does not fit
    // in the significand type, ldexp rounds it.
    BOOST_TEST(ldexp(3_DF, 4) == 48_DF);
    BOOST_TEST(ldexp(3_DF, -2) == 0.75_DF);
    BOOST_TEST(ldexp(9999999_DF, 1) == 2e7_DF);
    BOOST_TEST(ldexp(9999999_DF, -1) == 5e6_DF);
    BOOST_TEST(ldexp(9999999_DF, -4) == 624999.9_DF);
    BOOST_TEST(ldexp(9999999999999999_DD, 1) == 2e16_DD);
    BOOST_TEST(ldexp(9999999999999999_DD, -3) == 1.25e15_DD);
    BOOST_TEST(ldexp(1234567890123_DL, -11) == 602816352.59912109375_DL);
    BOOST_TEST(ldexp(123456789_DL, -21) == 58.868784427642822265625_DL);
    BOOST_TEST(ldexp(9999999999999999999999999999999999_DL, 1) == 2e34_DL);
    BOOST_TEST(ldexp(9999999999999999999999999999999999_DL, -1) == 5e33_DL);

    // Subnormal inputs and results
    BOOST_TEST(ldexp(5e-101_DF, 3) == 4e-100_DF);
    BOOST_TEST(ldexp(1e-95_DF, -20) == 1e-101_DF);
    BOOST_TEST(ldexp(3e-101_DF, -1) == 2e-101_DF);
    BOOST_TEST(ldexp(1e-101_DF, -4) == 0);
    BOOST_TEST(ldexp(1e-6176_DL, -1) == 0);
    BOOST_TEST(ldexp(1e-6176_DL, -2) == 0);
    BOOST_TEST(ldexp(3e-6176_DL, -2) == 1e-6176_DL);

    // Overflow, also with a huge e
    const auto big { ldexp(-9e96_DF, 1) };
    BOOST_TEST(isinf(big) && signbit(big));
    BOOST_TEST(isinf(ldexp(9.536743e90_DF, 20)));
    BOOST_TEST(isinf(ldexp(1e-101_DF, (std::numeric_limits<int>::max)())));

    // Underflow and an infinity keep the sign.
    const auto tiny { ldexp(-1e96_DF, (std::numeric_limits<int>::min)()) };
    BOOST_TEST((tiny == 0) && signbit(tiny));
    const auto neg_inf { ldexp(-std::numeric_limits<decimal32_t>::infinity(), 3) };
    BOOST_TEST(isinf(neg_inf) && signbit(neg_inf));

    // fesetround has an effect only with the detection of constant evaluation
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // Half of an odd value is a tie. fe_dec_to_nearest gives the even digit,
    // and fe_dec_to_nearest_from_zero rounds away from zero.
    fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest);
    BOOST_TEST(ldexp(1_DF, -30) == 9.313226e-10_DF);
    BOOST_TEST(ldexp(2000001_DF, -1) == 1000000_DF);
    BOOST_TEST(ldexp(-2000001_DF, -1) == -1000000_DF);
    BOOST_TEST(ldexp(2000003_DF, -1) == 1000002_DF);
    BOOST_TEST(ldexp(2000000000000001_DD, -1) == 1000000000000000_DD);
    BOOST_TEST(ldexp(2000000000000000000000000000000001_DL, -1) == 1000000000000000000000000000000000_DL);
    BOOST_TEST(ldexp(1e-101_DF, -1) == 0);

    fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST(ldexp(1_DF, -30) == 9.313226e-10_DF);
    BOOST_TEST(ldexp(2000001_DF, -1) == 1000001_DF);
    BOOST_TEST(ldexp(-2000001_DF, -1) == -1000001_DF);
    BOOST_TEST(ldexp(2000003_DF, -1) == 1000002_DF);
    BOOST_TEST(ldexp(2000000000000001_DD, -1) == 1000000000000001_DD);
    BOOST_TEST(ldexp(2000000000000000000000000000000001_DL, -1) == 1000000000000000000000000000000001_DL);
    BOOST_TEST(ldexp(1e-101_DF, -1) == 1e-101_DF);

    fesetround(boost::decimal::rounding_mode::fe_dec_upward);
    BOOST_TEST(ldexp(1_DF, -30) == 9.313226e-10_DF);
    BOOST_TEST(ldexp(-1_DF, -30) == -9.313225e-10_DF);
    BOOST_TEST(ldexp(1e-101_DF, -400) == 1e-101_DF);
    BOOST_TEST(ldexp(1e-101_DF, -4) == 1e-101_DF);

    fesetround(boost::decimal::rounding_mode::fe_dec_downward);
    BOOST_TEST(ldexp(1_DF, -30) == 9.313225e-10_DF);
    BOOST_TEST(ldexp(-1e-101_DF, -400) == -1e-101_DF);

    fesetround(boost::decimal::rounding_mode::fe_dec_toward_zero);
    BOOST_TEST(ldexp(-1_DF, -30) == -9.313225e-10_DF);
    BOOST_TEST(ldexp(9e96_DF, 1) == (std::numeric_limits<decimal32_t>::max)());

    fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest);
    #endif

    return errors_before == boost::detail::test_errors();
  }
}

auto main() -> int
{
  auto result_is_ok =
  (
       local::test_frexp_ldexp()
    && local::test_frexp_ldexp_exact()
    && local::test_frexp_edge()
    && local::test_ldexp_edge()
    && local::test_ldexp_exact()
  );

  result_is_ok = ((boost::report_errors() == 0) && result_is_ok);

  return (result_is_ok ? 0 : -1);
}

template<typename DecimalType> auto my_zero() -> DecimalType& { using decimal_type = DecimalType; static decimal_type val_zero { 0 }; return val_zero; }
template<typename DecimalType> auto my_one () -> DecimalType& { using decimal_type = DecimalType; static decimal_type val_one  { 1 }; return val_one; }
template<typename DecimalType> auto my_inf () -> DecimalType& { using decimal_type = DecimalType; static decimal_type val_inf  { std::numeric_limits<decimal_type>::infinity() }; return val_inf; }
template<typename DecimalType> auto my_nan () -> DecimalType& { using decimal_type = DecimalType; static decimal_type val_nan  { std::numeric_limits<decimal_type>::quiet_NaN() }; return val_nan; }
