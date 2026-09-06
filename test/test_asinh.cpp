// Copyright 2023 Matt Borland
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

  auto test_asinh(const std::int32_t tol_factor, const bool negate, const double range_lo, const double range_hi) -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

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
      const auto x_flt_begin = dis(gen);

      const auto x_flt = (negate ? -x_flt_begin : x_flt_begin);
      const auto x_dec = static_cast<decimal_type>(x_flt);

      using std::asinh;

      const auto val_flt = asinh(x_flt);
      const auto val_dec = asinh(x_dec);

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

  auto test_asinh_edge() -> bool
  {
    using decimal_type = boost::decimal::decimal32_t;

    std::mt19937_64 gen;

    std::uniform_real_distribution<float> dist(1.01F, 1.04F);

    auto result_is_ok = true;

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_nan_pos = asinh(std::numeric_limits<decimal_type>::quiet_NaN() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_nan_pos_is_ok = isnan(val_nan_pos) && (!signbit(val_nan_pos));

      BOOST_TEST(result_val_nan_pos_is_ok);

      result_is_ok = (result_val_nan_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_nan_neg = asinh(-std::numeric_limits<decimal_type>::quiet_NaN() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_nan_neg_is_ok = isnan(val_nan_neg) && (signbit(val_nan_neg));

      BOOST_TEST(result_val_nan_neg_is_ok);

      result_is_ok = (result_val_nan_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_pos = asinh(std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_pos_is_ok = (isinf(val_inf_pos) && (!signbit(val_inf_pos)));

      BOOST_TEST(result_val_inf_pos_is_ok);

      result_is_ok = (result_val_inf_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_inf_neg = asinh(-std::numeric_limits<decimal_type>::infinity() * static_cast<decimal_type>(dist(gen)));

      const auto result_val_inf_neg_is_ok = (isinf(val_inf_neg) && signbit(val_inf_neg));

      BOOST_TEST(result_val_inf_neg_is_ok);

      result_is_ok = (result_val_inf_neg_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_pos = asinh(::my_zero());

      const auto result_val_zero_pos_is_ok = ((fpclassify(val_zero_pos) == FP_ZERO) && (!signbit(val_zero_pos)));

      BOOST_TEST(result_val_zero_pos_is_ok);

      result_is_ok = (result_val_zero_pos_is_ok && result_is_ok);
    }

    for(auto i = static_cast<unsigned>(UINT8_C(0)); i < static_cast<unsigned>(UINT8_C(4)); ++i)
    {
      static_cast<void>(i);

      const auto val_zero_neg = asinh(-::my_zero());

      const auto result_val_zero_neg_is_ok = ((fpclassify(val_zero_neg) == FP_ZERO) && signbit(val_zero_neg));

      BOOST_TEST(result_val_zero_neg_is_ok);

      result_is_ok = (result_val_zero_neg_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  template<typename DecimalType>
  auto test_asinh_symmetry() -> bool
  {
    using decimal_type = DecimalType;
    using str_arg_array_type = std::array<const char*, 9U>;

    const str_arg_array_type arg_strings =
    {{
      "0.008", "0.009", "0.01",
      "0.39", "0.399", "0.4", "0.401", "0.41", "0.45"
    }};

    bool result_is_ok { true };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_arg_array_type>::value; ++i)
    {
      decimal_type x_arg { };

      static_cast<void>(from_chars(arg_strings[i], arg_strings[i] + std::strlen(arg_strings[i]), x_arg));

      const auto value_pos = asinh(x_arg);
      const auto value_neg = asinh(-x_arg);

      const auto result_value_is_ok = (value_neg == -value_pos);

      result_is_ok = (result_value_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  // These control values cover the range where this function gives an argument
  // to log1p that is larger than its own argument. The random tests use
  // decimal32_t and compare with float. Thus they cannot find an error that is
  // smaller than the epsilon of float.
  template<typename DecimalType>
  auto test_asinh_ctrl(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 28U>;

    const str_ctrl_array_type x_strings =
    {{
      "0.00000001", "0.00000002", "0.00000005", "0.00000008",
      "0.0001", "0.0002", "0.0005", "0.0008",
      "0.01", "0.02", "0.05", "0.08",
      "-0.45", "-0.35", "0.15",  "0.2",
      "0.25",  "0.3",   "0.35",  "0.4",
      "0.45",  "0.5",   "0.6", "0.8",
      "2.5",  "3.5",   "4.5",  "5.5"
    }};

    const str_ctrl_array_type ctrl_strings =
    {{
      // Table[N[ArcSinh[x], 36], <each string in x_strings>]
      "9.99999999999999983333333333333334083E-9",
      "1.99999999999999986666666666666669067E-8",
      "4.99999999999999791666666666666901042E-8",
      "7.99999999999999146666666666669124267E-8",
      "0.0000999999998333333340833333288690476494",
      "0.000199999998666666690666666095238110794",
      "0.000499999979166669010416317894404577820",
      "0.000799999914666691242657304385030174586",
      "0.00999983334083288693514161216395379598",
      "0.0199986669066095393604989679785597966",
      "0.0499791900693486652313949621363567866",
      "0.0799149114944967681643977917215557312",
      "-0.436049668851740526505395726650547201",
      "-0.343221555085943962127800239950427517",
      "0.149443120184957656160285809150591369",
      "0.198690110349241406474636915950206968",
      "0.247466461547263452944781549788359289",
      "0.295673047563422439102710529733517082",
      "0.343221555085943962127800239950427517",
      "0.390035319770715276080163379883629645",
      "0.436049668851740526505395726650547201",
      "0.481211825059603447497758913424368423",
      "0.568824898732247530098688336861388356",
      "0.732668256045410864154917816040544816",
      "1.64723114637109571062485861044361966",
      "1.96572047164965152123875634867006930",
      "2.20934770861533427770567648090580700",
      "2.40605912529801723748879456712184212",
    }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(x_strings[i], x_strings[i] + std::strlen(x_strings[i]), x_arg));
      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      const decimal_type value { asinh(x_arg) };

      const auto result_asinh_is_ok = is_close_fraction(value, ctrl_value, my_tol);

      result_is_ok = (result_asinh_is_ok && result_is_ok);
    }

    return result_is_ok;
  }

  template<typename DecimalType>
  auto test_asinh_ranges(const int tol_factor) -> bool
  {
    using decimal_type = DecimalType;

    using str_ctrl_array_type = std::array<const char*, 21U>;

    const str_ctrl_array_type ctrl_strings =
    {{
      // Table[N[ArcSinh[(n  10 + 1)/10], 36], {n, 0, 20, 1}]
      "0.0998340788992075633273031247047694433",
      "0.950346929821134250270071594269894406",
      "1.48748283664127117363072352420668941",
      "1.84960367947317840340492402262497951",
      "2.11868503933338489611928581943378992",
      "2.33186371852126593632726171542586273",
      "2.50808785933768270069328278606856154",
      "2.65816480695413889429737823065299529",
      "2.78880004092017897657297263623866615",
      "2.90442697297147614516955384100360968",
      "3.00812438409137047610811262257570901",
      "3.10211519705688481315971228719445454",
      "3.18805580981569795899481687954141334",
      "3.26721302923313181165492165705455780",
      "3.34057709467676152986028455201330024",
      "3.40893656861911045850074444733130038",
      "3.47292952919612719274592230188162122",
      "3.53307951362261773269566946001994917",
      "3.58982134929134256204737895589037993",
      "3.64352010048564228442166720182722257",
      "3.69448521861416359945802758262176555"
    }};

    bool result_is_ok { true };

    const decimal_type my_tol { std::numeric_limits<decimal_type>::epsilon() * static_cast<decimal_type>(tol_factor) };

    for(auto i = static_cast<std::size_t>(UINT8_C(0)); i < std::tuple_size<str_ctrl_array_type>::value; ++i)
    {
      decimal_type x_arg      { ((i * 10U) + 1U), -1 };
      decimal_type ctrl_value { };

      static_cast<void>(from_chars(ctrl_strings[i], ctrl_strings[i] + std::strlen(ctrl_strings[i]), ctrl_value));

      const decimal_type value { asinh(x_arg) };

      const auto result_asinh_is_ok = is_close_fraction(value, ctrl_value, my_tol);

      result_is_ok = (result_asinh_is_ok && result_is_ok);
    }

    return result_is_ok;
  }
} // namespace local

auto main() -> int
{
  auto result_is_ok = true;

  const auto result_tiny_is_ok       = local::test_asinh(INT32_C(48), false, 1.001, 1.1);         // High tolerance due to float conversion for comparison.
  const auto result_small_is_ok      = local::test_asinh(INT32_C(48), false, 0.1, 1.59);          // High tolerance due to float conversion for comparison.
  const auto result_small_neg_is_ok  = local::test_asinh(INT32_C(48), true,  0.1, 1.59);          // High tolerance due to float conversion for comparison.
  const auto result_large_is_ok      = local::test_asinh(INT32_C(48), false, 1.0E+01, 1.0E+19);   // High tolerance due to float conversion for comparison.

  BOOST_TEST(result_tiny_is_ok);
  BOOST_TEST(result_small_is_ok);
  BOOST_TEST(result_small_neg_is_ok);
  BOOST_TEST(result_large_is_ok);

  const auto result_edge_is_ok  = local::test_asinh_edge();

  result_is_ok =
  (
       result_tiny_is_ok
    && result_small_is_ok
    && result_large_is_ok
    && result_edge_is_ok
    && result_is_ok
  );

  {
    using namespace boost::decimal;

    const bool result_asinh_symmetry_dec32_t_is_ok       = local::test_asinh_symmetry<decimal32_t>();
    const bool result_asinh_symmetry_dec64_t_is_ok       = local::test_asinh_symmetry<decimal64_t>();
    const bool result_asinh_symmetry_dec128_t_is_ok      = local::test_asinh_symmetry<decimal128_t>();
    const bool result_asinh_symmetry_dec_fast32_t_is_ok  = local::test_asinh_symmetry<decimal_fast32_t>();
    const bool result_asinh_symmetry_dec_fast64_t_is_ok  = local::test_asinh_symmetry<decimal_fast64_t>();
    const bool result_asinh_symmetry_dec_fast128_t_is_ok = local::test_asinh_symmetry<decimal_fast128_t>();

    BOOST_TEST(result_asinh_symmetry_dec32_t_is_ok);
    BOOST_TEST(result_asinh_symmetry_dec64_t_is_ok);
    BOOST_TEST(result_asinh_symmetry_dec128_t_is_ok);
    BOOST_TEST(result_asinh_symmetry_dec_fast32_t_is_ok);
    BOOST_TEST(result_asinh_symmetry_dec_fast64_t_is_ok);
    BOOST_TEST(result_asinh_symmetry_dec_fast128_t_is_ok);

    const bool result_asinh_dec32_t_is_ok       = local::test_asinh_ctrl<decimal32_t>      (16);
    const bool result_asinh_dec64_t_is_ok       = local::test_asinh_ctrl<decimal64_t>      (16);
    const bool result_asinh_dec128_t_is_ok      = local::test_asinh_ctrl<decimal128_t>     (16);
    const bool result_asinh_dec_fast32_t_is_ok  = local::test_asinh_ctrl<decimal_fast32_t> (16);
    const bool result_asinh_dec_fast64_t_is_ok  = local::test_asinh_ctrl<decimal_fast64_t> (16);
    const bool result_asinh_dec_fast128_t_is_ok = local::test_asinh_ctrl<decimal_fast128_t>(16);

    BOOST_TEST(result_asinh_dec32_t_is_ok);
    BOOST_TEST(result_asinh_dec64_t_is_ok);
    BOOST_TEST(result_asinh_dec128_t_is_ok);
    BOOST_TEST(result_asinh_dec_fast32_t_is_ok);
    BOOST_TEST(result_asinh_dec_fast64_t_is_ok);
    BOOST_TEST(result_asinh_dec_fast128_t_is_ok);
  }

  {
    using namespace boost::decimal;

    const bool result_asinh_ranges_dec32_t_is_ok       = local::test_asinh_ranges<decimal32_t>      (16);
    const bool result_asinh_ranges_dec64_t_is_ok       = local::test_asinh_ranges<decimal64_t>      (16);
    const bool result_asinh_ranges_dec128_t_is_ok      = local::test_asinh_ranges<decimal128_t>     (16);
    const bool result_asinh_ranges_dec_fast32_t_is_ok  = local::test_asinh_ranges<decimal_fast32_t> (16);
    const bool result_asinh_ranges_dec_fast64_t_is_ok  = local::test_asinh_ranges<decimal_fast64_t> (16);
    const bool result_asinh_ranges_dec_fast128_t_is_ok = local::test_asinh_ranges<decimal_fast128_t>(16);

    BOOST_TEST(result_asinh_ranges_dec32_t_is_ok);
    BOOST_TEST(result_asinh_ranges_dec64_t_is_ok);
    BOOST_TEST(result_asinh_ranges_dec128_t_is_ok);
    BOOST_TEST(result_asinh_ranges_dec_fast32_t_is_ok);
    BOOST_TEST(result_asinh_ranges_dec_fast64_t_is_ok);
    BOOST_TEST(result_asinh_ranges_dec_fast128_t_is_ok);
  }

  BOOST_TEST(result_is_ok);

  return boost::report_errors();
}

auto my_zero() -> boost::decimal::decimal32_t& { static boost::decimal::decimal32_t val_zero { 0, 0 }; return val_zero; }
