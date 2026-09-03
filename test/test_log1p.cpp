// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
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

auto my_zero() -> boost::decimal::decimal32_t&;
auto my_one () -> boost::decimal::decimal32_t&;

auto my_make_nan(boost::decimal::decimal32_t factor) -> boost::decimal::decimal32_t;

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

    NumericType delta { };

    if(b == static_cast<NumericType>(0))
    {
      delta = fabs(a - b); // LCOV_EXCL_LINE

      result_is_ok = (delta < tol); // LCOV_EXCL_LINE
    }
    else
    {
      delta = fabs(1 - (a / b));

      result_is_ok = (delta < tol);
    }

    // LCOV_EXCL_START
    if (!result_is_ok)
    {
      std::cerr << std::setprecision(std::numeric_limits<NumericType>::digits10) << "a: " << a
                << "\nb: " << b
                << "\ndelta: " << delta
                << "\ntol: " << tol << std::endl;
    }
    // LCOV_EXCL_STOP

    return result_is_ok;
  }

  auto test_log1p(const int tol_factor, const bool negate, const long double range_lo, const long double range_hi) -> bool
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

      using std::log1p;

      const auto val_flt = log1p(x_flt);
      const auto val_dec = log1p(x_dec);

      const auto result_val_is_ok = is_close_fraction(val_flt, static_cast<float>(val_dec), std::numeric_limits<float>::epsilon() * static_cast<float>(tol_factor));

      result_is_ok = (result_val_is_ok && result_is_ok);

      if(!result_val_is_ok)
      {
        // LCOV_EXCL_START
        std::cerr << "x_flt  : " <<                    x_flt   << std::endl;
        std::cerr << "val_flt: " << std::scientific << val_flt << std::endl;
        std::cerr << "val_dec: " << std::scientific << val_dec << std::endl;

        break;
        // LCOV_EXCL_STOP
      }
    }

    BOOST_TEST(result_is_ok);

    return result_is_ok;
  }

  auto test_log1p_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    std::uniform_real_distribution<float> dist(1.01F, 1.04F);

    auto result_is_ok = true;

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(64)); ++i)
    {
      static_cast<void>(i);

      const auto arg_nan = my_make_nan(static_cast<decimal_type>(dist(gen)));

      const auto val_nan = log1p(arg_nan);

      const auto result_val_nan_is_ok = (isnan(arg_nan) && isnan(val_nan));

      BOOST_TEST(result_val_nan_is_ok);

      result_is_ok = (result_val_nan_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_pos = log1p(std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_pos_is_ok = isinf(val_inf_pos);

      BOOST_TEST(result_val_inf_pos_is_ok);

      result_is_ok = (result_val_inf_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_neg = log1p(-std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_neg_is_ok = isnan(val_inf_neg);

      BOOST_TEST(result_val_inf_neg_is_ok);

      result_is_ok = (result_val_inf_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_pos = log1p(::my_zero());

      const auto result_val_zero_pos_is_ok = (val_zero_pos == ::my_zero());

      BOOST_TEST(result_val_zero_pos_is_ok);

      result_is_ok = (result_val_zero_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_neg = log1p(-::my_zero());

      const auto result_val_zero_neg_is_ok = (-val_zero_neg == ::my_zero());

      BOOST_TEST(result_val_zero_neg_is_ok);

      result_is_ok = (result_val_zero_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_one_minus = log1p(-::my_one());

      const auto result_val_one_minus_is_ok = (isinf(val_one_minus) && signbit(val_one_minus));

      BOOST_TEST(result_val_one_minus_is_ok);

      result_is_ok = (result_val_one_minus_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_something_minus = log1p(-(::my_one() + ::my_one()) * static_cast<decimal_type>(dist(gen)));

      const auto result_val_something_minus_is_ok = isnan(val_something_minus);

      BOOST_TEST(result_val_something_minus_is_ok);

      result_is_ok = (result_val_something_minus_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  auto test_log1p_64(const int tol_factor) -> bool
  {
    using decimal_type = boost::decimal::decimal64_t;

    using val_ctrl_array_type = std::array<double, 21U>;

    const val_ctrl_array_type ctrl_values =
    {{
      // Table[N[Log[1 + n/100], 17], {n, -10, 10, 1}]
      -0.10536051565782630,  -0.094310679471241327, -0.083381608939051058,
      -0.072570692834835431, -0.061875403718087472, -0.051293294387550533,
      -0.040821994520255130, -0.030459207484708546, -0.020202707317519448,
      -0.010050335853501441,  0,                     0.0099503308531680828,
       0.019802627296179713,  0.029558802241544403,  0.039220713153281296,
       0.048790164169432003,  0.058268908123975776,  0.067658648473814805,
       0.076961041136128325,  0.086177696241052332,  0.095310179804324860
    }};

    std::array<decimal_type, std::tuple_size<val_ctrl_array_type>::value> log1p_values { };

    int nx { -10 };

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<val_ctrl_array_type>::value; ++i)
    {
      // Table[N[Log[1 + n/100], 17], {n, -1, 10, 1}]

      const decimal_type x_arg { nx, -2 };

      log1p_values[i] = log1p(x_arg);

      ++nx;

      const auto result_log1p_is_ok = is_close_fraction(log1p_values[i], decimal_type(ctrl_values[i]), my_tol);

      result_is_ok = (result_log1p_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  auto test_log1p_128(const int tol_factor) -> bool
  {
    using decimal_type = boost::decimal::decimal128_t;

    using str_ctrl_array_type = std::array<const char*, 21U>;

    const str_ctrl_array_type ctrl_strings =
    {{
        // Table[N[Log[1 + n/100], 36], {n, -1, 10, 1}]
       "-0.105360515657826301227500980839312798",
       "-0.0943106794712413268771427243602300808",
       "-0.0833816089390510583947658346421791606",
       "-0.0725706928348354307115733479038455001",
       "-0.0618754037180874717978001181383781382",
       "-0.0512932943875505334261961442546872384",
       "-0.0408219945202551295545770651553198702",
       "-0.0304592074847085459192612876647667014",
       "-0.0202027073175194484080453010241923879",
       "-0.0100503358535014411835488575585477061",
       "0",
       "0.00995033085316808284821535754426074169",
       "0.0198026272961797130260290668851003931",
       "0.0295588022415444027326194056847124054",
       "0.0392207131532812962692008965711198938",
       "0.0487901641694320030653744042231646586",
       "0.0582689081239757755257183511185059232",
       "0.0676586484738148052684159076545485864",
       "0.0769610411361283249842170443152018349",
       "0.0861776962410523323413335428404732359",
       "0.0953101798043248600439521232807650922",
    }};

    std::array<decimal_type, std::tuple_size<str_ctrl_array_type>::value> log1p_values { };
    std::array<decimal_type, std::tuple_size<str_ctrl_array_type>::value> ctrl_values { };

    int nx { -10 };

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      const decimal_type x_arg { nx, -2 };

      ++nx;

      log1p_values[i] = log1p(x_arg);

      static_cast<void>
      (
        from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_values[i])
      );

      const auto result_log1p_is_ok = is_close_fraction(log1p_values[i], ctrl_values[i], my_tol);

      result_is_ok = (result_log1p_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  // These control values cover the full range of the series branch, for all
  // six types. The random tests use decimal32_t and compare with float. Thus
  // they cannot find an error that is smaller than the epsilon of float.
  template<typename DecimalType>
  auto test_log1p_wide(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 32U>;

    const str_ctrl_array_type x_strings =
    {{
      "-0.999", "-0.99",  "-0.95",  "-0.9",
      "-0.8",   "-0.75",  "-0.7",   "-0.6",
      "-0.5",   "-0.45",  "-0.4",   "-0.35",
      "-0.3",   "-0.25",  "-0.2",   "-0.15",
      "-1e-12", "-1e-25", "1e-25",  "1e-12",
      "0.15",   "0.2",    "0.25",   "0.3",
      "0.35",   "0.4",    "0.45",   "0.5",
      "0.55",   "0.75",   "1.0",    "2.0",
    }};

    const str_ctrl_array_type ctrl_strings =
    {{
      // Table[N[Log[1 + x], 36], {x, x_strings}]
      "-6.90775527898213705205397436405309262",
      "-4.60517018598809136803598290936872842",
      "-2.99573227355399099343522357614254078",
      "-2.30258509299404568401799145468436421",
      "-1.60943791243410037460075933322618764",
      "-1.38629436111989061883446424291635314",
      "-1.2039728043259359926227462177618385",
      "-0.916290731874155065183527211768011071",
      "-0.693147180559945309417232121458176568",
      "-0.597837000755620449373279998177411476",
      "-0.510825623765990683205514096303661935",
      "-0.430782916092454257381736134577222171",
      "-0.356674943938732378912638711241184478",
      "-0.287682072451780927439219005993827432",
      "-0.223143551314209755766295090309834503",
      "-0.16251892949777491318568895826941424",
      "-0.00000000000100000000000050000000000033333333333",
      "-0.000000000000000000000000100000000000000000000000005",
      "0.000000000000000000000000099999999999999999999999995",
      "0.000000000000999999999999500000000000333333333333",
      "0.139761942375158697371529255667655343",
      "0.182321556793954626211718025154514633",
      "0.223143551314209755766295090309834503",
      "0.262364264467491052035495986880954397",
      "0.300104592450338080750512134625036338",
      "0.33647223662121293050459341021699209",
      "0.37156355643248303374804845621937083",
      "0.405465108108164381978013115464349137",
      "0.438254930931155252493940748399816435",
      "0.559615787935422686270888500526826593",
      "0.693147180559945309417232121458176568",
      "1.0986122886681096913952452369225257",
    }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(x_strings[i], x_strings[i] + std::strlen(x_strings[i]), x_arg));
      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      const auto result_log1p_is_ok = is_close_fraction(log1p(x_arg), ctrl_value, my_tol);

      result_is_ok = (result_log1p_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  template<typename DecimalType>
  auto test_log1p_monotonic(const char* type_name, const int pair_count) -> bool
  {
    using decimal_type = DecimalType;

    // log1p increases at every argument, thus two adjacent values must not give
    // results in the opposite order. Each walk stays below 0.5, the join of the two
    // branches, which has a pair of its own that this test does not cover.

    using start_array_type = std::array<decimal_type, 6U>;

    const start_array_type start_values =
    {{
      decimal_type {  5, -2 }, decimal_type {  1, -1 }, decimal_type {  2, -1 },
      decimal_type {  3, -1 }, decimal_type {  4, -1 }, decimal_type { 45, -2 },
    }};

    const decimal_type one { 1, 0 };

    auto inversion_count = static_cast<std::uintmax_t>(UINT8_C(0));

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<start_array_type>::value; ++i)
    {
      decimal_type z    { start_values[i] };
      decimal_type prev { log1p(z) };

      for(auto n = 0; n < pair_count; ++n)
      {
        z = nextafter(z, one);

        const decimal_type cur { log1p(z) };

        if(cur < prev)
        {
          ++inversion_count;
        }

        prev = cur;
      }
    }

    const bool result_is_ok { inversion_count == static_cast<std::uintmax_t>(UINT8_C(0)) };

    if(!result_is_ok)
    {
      std::cout << "log1p is not monotonic for " << type_name << ": " << inversion_count << " inversions" << std::endl;
    }

    return result_is_ok;
  }

} // namespace local

auto main() -> int
{
  auto result_is_ok = true;

  const auto result_pos_is_ok = local::test_log1p(96, false, 0.0L, 2.0L);

  const auto result_narrow_is_ok = local::test_log1p(16, false, -0.375L, 0.375L);

  const auto result_neg_is_ok = local::test_log1p(96, true, 0.0L, 0.95L);

  const auto result_pos_wide_is_ok = local::test_log1p(96, false, 1.0L, 1.0E6L);

  const auto result_edge_is_ok = local::test_log1p_edge();

  BOOST_TEST(result_pos_is_ok);
  BOOST_TEST(result_narrow_is_ok);
  BOOST_TEST(result_neg_is_ok);
  BOOST_TEST(result_pos_wide_is_ok);
  BOOST_TEST(result_edge_is_ok);

  result_is_ok = (result_pos_is_ok      && result_is_ok);
  result_is_ok = (result_narrow_is_ok   && result_is_ok);
  result_is_ok = (result_neg_is_ok      && result_is_ok);
  result_is_ok = (result_pos_wide_is_ok && result_is_ok);
  result_is_ok = (result_edge_is_ok     && result_is_ok);

  {
    const auto result_pos64_is_ok = local::test_log1p_64(64);

    BOOST_TEST(result_pos64_is_ok);

    result_is_ok = (result_pos64_is_ok && result_is_ok);
  }

  {
    const auto result_pos128_is_ok = local::test_log1p_128(8192);

    BOOST_TEST(result_pos128_is_ok);

    result_is_ok = (result_pos128_is_ok && result_is_ok);
  }

  {
    using namespace boost::decimal;

    const auto result_wide_is_ok =
    (
         local::test_log1p_wide<decimal32_t>      (16)
      && local::test_log1p_wide<decimal64_t>      (16)
      && local::test_log1p_wide<decimal128_t>     (16)
      && local::test_log1p_wide<decimal_fast32_t> (16)
      && local::test_log1p_wide<decimal_fast64_t> (16)
      && local::test_log1p_wide<decimal_fast128_t>(16)
    );

    BOOST_TEST(result_wide_is_ok);

    result_is_ok = (result_wide_is_ok && result_is_ok);
  }

  {
    using namespace boost::decimal;

    const auto result_mono_d32_is_ok   = local::test_log1p_monotonic<decimal32_t>      ("decimal32_t",       40000);
    const auto result_mono_d64_is_ok   = local::test_log1p_monotonic<decimal64_t>      ("decimal64_t",       40000);
    const auto result_mono_d128_is_ok  = local::test_log1p_monotonic<decimal128_t>     ("decimal128_t",       2000);
    const auto result_mono_f32_is_ok   = local::test_log1p_monotonic<decimal_fast32_t> ("decimal_fast32_t",  40000);
    const auto result_mono_f64_is_ok   = local::test_log1p_monotonic<decimal_fast64_t> ("decimal_fast64_t",  40000);
    const auto result_mono_f128_is_ok  = local::test_log1p_monotonic<decimal_fast128_t>("decimal_fast128_t",  2000);

    const auto result_monotonic_is_ok =
    (
         result_mono_d32_is_ok  && result_mono_d64_is_ok  && result_mono_d128_is_ok
      && result_mono_f32_is_ok  && result_mono_f64_is_ok  && result_mono_f128_is_ok
    );

    BOOST_TEST(result_monotonic_is_ok);

    result_is_ok = (result_monotonic_is_ok && result_is_ok);
  }

  result_is_ok = ((boost::report_errors() == 0) && result_is_ok);

  return (result_is_ok ? 0 : -1);
}

auto my_zero() -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_zero { 0, 0 }; return val_zero; }
auto my_one () -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_one  { 1, 0 }; return val_one; }

auto my_make_nan(boost::decimal::decimal32_t factor) -> boost::decimal::decimal32_t
{
  boost::decimal::decimal32_t val_nan = std::numeric_limits<boost::decimal::decimal32_t>::quiet_NaN();

  return val_nan * factor;
}
