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

  auto test_atanh(const std::int32_t tol_factor, const bool negate, const double range_lo, const double range_hi) -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::random_device rd;
    std::mt19937_64 gen(rd());

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
      const auto x_flt_begin = dis(gen);

      const auto x_flt = (negate ? -x_flt_begin : x_flt_begin);
      const auto x_dec = static_cast<decimal_type>(x_flt);

      using std::atanh;

      const auto val_flt = atanh(x_flt);
      const auto val_dec = atanh(x_dec);

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

  auto test_atanh_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    std::uniform_real_distribution<float> dist(1.01F, 1.04F);

    auto result_is_ok = true;

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_nan = atanh(std::numeric_limits<decimal_type>::quiet_NaN() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_nan_is_ok = isnan(val_nan);

      BOOST_TEST(result_val_nan_is_ok);

      result_is_ok = (result_val_nan_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_pos = atanh(std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_pos_is_ok = (isinf(val_inf_pos) && (!signbit(val_inf_pos)));

      BOOST_TEST(result_val_inf_pos_is_ok);

      result_is_ok = (result_val_inf_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_neg = atanh(-std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_neg_is_ok = (isinf(val_inf_neg) && signbit(val_inf_neg));

      BOOST_TEST(result_val_inf_neg_is_ok);

      result_is_ok = (result_val_inf_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_one_pos = atanh(::my_one());

      const auto result_val_one_pos_is_ok = (isinf(val_one_pos) && (!signbit(val_one_pos)));

      BOOST_TEST(result_val_one_pos_is_ok);

      result_is_ok = (result_val_one_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_one_neg = atanh(-::my_one());

      const auto result_val_one_neg_is_ok = (isinf(val_one_neg) && signbit(val_one_neg));

      BOOST_TEST(result_val_one_neg_is_ok);

      result_is_ok = (result_val_one_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero = atanh(::my_zero());

      const auto result_val_zero_is_ok = (fpclassify(val_zero) == FP_ZERO);

      BOOST_TEST(result_val_zero_is_ok);

      result_is_ok = (result_val_zero_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_gt_one_pos = atanh(::my_one() * static_cast<decimal_type>(dist(gen) * dist(gen) * dist(gen)));

      const auto result_val_gt_one_pos_is_ok = (isnan(val_gt_one_pos) && (!signbit(val_gt_one_pos)));

      BOOST_TEST(result_val_gt_one_pos_is_ok);

      result_is_ok = (result_val_gt_one_pos_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  // These control values cover the range where this function gives an argument
  // to log1p that is larger than its own argument. The random tests use
  // decimal32_t and compare with float. Thus they cannot find an error that is
  // smaller than the epsilon of float.
  template<typename DecimalType>
  auto test_atanh_ctrl(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 11U>;

    const str_ctrl_array_type x_strings =
    {{
      "-0.45", "-0.35", "-0.3",  "0.15",
      "0.2",   "0.25",  "0.3",   "0.35",
      "0.4",   "0.45",  "0.49",
    }};

    const str_ctrl_array_type ctrl_strings =
    {{
      // Table[N[ArcTanh[x], 36], {x, x_strings}]
      "-0.484700278594051741560664227198391153",
      "-0.365443754271396169066124134601129255",
      "-0.309519604203111715474067349061069438",
      "0.151140435936466805278609106968534791",
      "0.202732554054082190989006557732174568",
      "0.255412811882995341602757048151830967",
      "0.309519604203111715474067349061069438",
      "0.365443754271396169066124134601129255",
      "0.423648930193601806855053753260327012",
      "0.484700278594051741560664227198391153",
      "0.536060336610566684673824210154234124",
    }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(x_strings[i], x_strings[i] + std::strlen(x_strings[i]), x_arg));
      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      const auto result_atanh_is_ok = is_close_fraction(atanh(x_arg), ctrl_value, my_tol);

      result_is_ok = (result_atanh_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

} // namespace local

auto main() -> int
{
  auto result_is_ok = true;

  constexpr boost::decimal::decimal32_t fourth_root_epsilon { 1, -((std::numeric_limits<boost::decimal::decimal32_t>::digits10 + 1) / 4) };

  const auto result_eps_is_ok =
    local::test_atanh
    (
      INT32_C(128),
      false,
      static_cast<float>(static_cast<double>(fourth_root_epsilon) / 32.0),
      static_cast<float>(static_cast<double>(fourth_root_epsilon) * 32.0)
    );

  const auto result_eps_near_one_is_ok =
    local::test_atanh
    (
      INT32_C(256),
      false,
      static_cast<float>(static_cast<long double>( static_cast<float>(1.0L) - static_cast<float>(static_cast<double>(fourth_root_epsilon) * 32.0L))),
      static_cast<float>(static_cast<long double>( static_cast<float>(1.0L) - static_cast<float>(static_cast<double>(fourth_root_epsilon) / 32.0L)))
    );

  const auto result_tiny_is_ok       = local::test_atanh(INT32_C(96), false, 0.001, 0.1);
  const auto result_medium_is_ok     = local::test_atanh(INT32_C(96), true,  0.1, 0.9);
  const auto result_medium_neg_is_ok = local::test_atanh(INT32_C(96), false, 0.1, 0.9);

  BOOST_TEST(result_eps_is_ok);
  BOOST_TEST(result_eps_near_one_is_ok);
  BOOST_TEST(result_tiny_is_ok);
  BOOST_TEST(result_medium_is_ok);
  BOOST_TEST(result_medium_is_ok);

  const auto result_edge_is_ok  = local::test_atanh_edge();

  result_is_ok =
  (
       result_eps_is_ok
    && result_tiny_is_ok
    && result_medium_is_ok
    && result_medium_neg_is_ok
    && result_edge_is_ok
    && result_is_ok
  );

  {
    using namespace boost::decimal;

    const auto result_ctrl_is_ok =
    (
         local::test_atanh_ctrl<decimal32_t>      (16)
      && local::test_atanh_ctrl<decimal64_t>      (16)
      && local::test_atanh_ctrl<decimal128_t>     (16)
      && local::test_atanh_ctrl<decimal_fast32_t> (16)
      && local::test_atanh_ctrl<decimal_fast64_t> (16)
      && local::test_atanh_ctrl<decimal_fast128_t>(16)
    );

    BOOST_TEST(result_ctrl_is_ok);

    result_is_ok = (result_ctrl_is_ok && result_is_ok);
  }

  BOOST_TEST(result_is_ok);

  return boost::report_errors();
}

auto my_zero() -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_zero { 0, 0 }; return val_zero; }
auto my_one () -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_one  { 1, 0 }; return val_one; }
