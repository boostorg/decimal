// Copyright 2024 Matt Borland
// Copyright 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_ATAN_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_ATAN_IMPL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point_series.hpp>
#include <boost/decimal/detail/construction_sign.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

namespace atan_detail {

// Indices into atan_values
enum : std::size_t { atan_half_hi, atan_half_lo, atan_three_halves_hi, atan_three_halves_lo };

// Use a struct of arrays so that we can have static constexpr arrays of coefficients.
// See https://github.com/boostorg/math/issues/923 for further information
template <bool b>
struct atan_table_imp
{
    // Chebyshev fits of P in atan(x) = x + x^3 P(x^2) for x in [0, 0.4375], highest power first, as
    // raw fixed point (see fixed_point_series.hpp): n = round(c * 2^62) or n = round(c * 2^126)

    // Q62, degree 4 in x^2, max kernel error 0.0185 ulp at 7 digits
    static constexpr std::array<std::int64_t, 5> d32_coeffs = {{
        INT64_C(-286653686818871604),
        INT64_C(491992650021305608),
        INT64_C(-657468245397313456),
        INT64_C(922305383051950432),
        INT64_C(-1537228519086262020)
    }};

    // Q62, degree 11 in x^2, max kernel error 0.027 ulp at 16 digits
    static constexpr std::array<std::int64_t, 12> d64_coeffs = {{
        INT64_C(68129288945790802),
        INT64_C(-152776553565301478),
        INT64_C(207168893735716832),
        INT64_C(-240576890164062181),
        INT64_C(271026318056636923),
        INT64_C(-307426146734136447),
        INT64_C(354744057432338816),
        INT64_C(-419244149583371944),
        INT64_C(512409556935913322),
        INT64_C(-658812288339968584),
        INT64_C(922337203685450372),
        INT64_C(-1537228672809129148)
    }};

    // Q126, degree 24 in x^2, max kernel error 0.0314 ulp at 34 digits
    static constexpr std::array<int128::int128_t, 25> d128_coeffs = {{
        int128::int128_t {INT64_C(-10485371323887896), UINT64_C(10517237177405548740)},
        int128::int128_t {INT64_C(36037310788783410), UINT64_C(17838482110476103184)},
        int128::int128_t {INT64_C(-65868101180795099), UINT64_C(11297532328258753760)},
        int128::int128_t {INT64_C(88709017543273301), UINT64_C(10070282870844457804)},
        int128::int128_t {INT64_C(-102651823325949239), UINT64_C(12908569194395175164)},
        int128::int128_t {INT64_C(111261874313837669), UINT64_C(1207799242924094493)},
        int128::int128_t {INT64_C(-117988633709135858), UINT64_C(11530285581856098480)},
        int128::int128_t {INT64_C(124595196758679357), UINT64_C(4918781308014821961)},
        int128::int128_t {INT64_C(-131756095136795816), UINT64_C(4568656608273312686)},
        int128::int128_t {INT64_C(139747322689771072), UINT64_C(2293705908972136111)},
        int128::int128_t {INT64_C(-148763994724226424), UINT64_C(13130196723226486758)},
        int128::int128_t {INT64_C(159023650305606894), UINT64_C(16383032007635900459)},
        int128::int128_t {INT64_C(-170803185516202007), UINT64_C(10439900632158261928)},
        int128::int128_t {INT64_C(184467440718861643), UINT64_C(10470388862571930467)},
        int128::int128_t {INT64_C(-200508087756951228), UINT64_C(6100221296306786481)},
        int128::int128_t {INT64_C(219604096115564635), UINT64_C(2688867774528155709)},
        int128::int128_t {INT64_C(-242720316759335550), UINT64_C(391790395505276335)},
        int128::int128_t {INT64_C(271275648142787510), UINT64_C(14028508071849608541)},
        int128::int128_t {INT64_C(-307445734561825861), UINT64_C(17059997906172122564)},
        int128::int128_t {INT64_C(354745078340568300), UINT64_C(5638858341042803538)},
        int128::int128_t {INT64_C(-419244183493398901), UINT64_C(11739098548382091930)},
        int128::int128_t {INT64_C(512409557603043100), UINT64_C(8198551786701958321)},
        int128::int128_t {INT64_C(-658812288346769701), UINT64_C(7905747462781355944)},
        int128::int128_t {INT64_C(922337203685477580), UINT64_C(14757395258965233795)},
        int128::int128_t {INT64_C(-1537228672809129302), UINT64_C(12297829382473037246)}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal32_t, 4> d32_values = {{
        decimal32_t {UINT64_C(4636476), -7},
        decimal32_t {UINT64_C(9000806), -15},
        decimal32_t {UINT64_C(9827937), -7},
        decimal32_t {UINT64_C(2324733), -14}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal_fast32_t, 4> d32_fast_values = {{
        decimal_fast32_t {UINT64_C(4636476), -7},
        decimal_fast32_t {UINT64_C(9000806), -15},
        decimal_fast32_t {UINT64_C(9827937), -7},
        decimal_fast32_t {UINT64_C(2324733), -14}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal64_t, 4> d64_values = {{
        decimal64_t {UINT64_C(4636476090008061), -16},
        decimal64_t {UINT64_C(1621425623146121), -32},
        decimal64_t {UINT64_C(9827937232473291), -16},
        decimal64_t {UINT64_C(3201428938898533), -32, construction_sign::negative}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal_fast64_t, 4> d64_fast_values = {{
        decimal_fast64_t {UINT64_C(4636476090008061), -16},
        decimal_fast64_t {UINT64_C(1621425623146121), -32},
        decimal_fast64_t {UINT64_C(9827937232473291), -16},
        decimal_fast64_t {UINT64_C(3201428938898533), -32, construction_sign::negative}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal128_t, 4> d128_values = {{
        decimal128_t {int128::uint128_t{UINT64_C(251343872473191), UINT64_C(15780610568723885488)}, -34},
        decimal128_t {int128::uint128_t{UINT64_C(109967214061217), UINT64_C(15895449937643443526)}, -69},
        decimal128_t {int128::uint128_t{UINT64_C(53277354492493), UINT64_C(10864265368342995978)}, -33},
        decimal128_t {int128::uint128_t{UINT64_C(78587730147417), UINT64_C(12843742829858255927)}, -68}
    }};

    // atan(1/2) and atan(3/2), each as a rounded high part and the low part left over
    static constexpr std::array<decimal_fast128_t, 4> d128_fast_values = {{
        decimal_fast128_t {int128::uint128_t{UINT64_C(251343872473191), UINT64_C(15780610568723885488)}, -34},
        decimal_fast128_t {int128::uint128_t{UINT64_C(109967214061217), UINT64_C(15895449937643443526)}, -69},
        decimal_fast128_t {int128::uint128_t{UINT64_C(53277354492493), UINT64_C(10864265368342995978)}, -33},
        decimal_fast128_t {int128::uint128_t{UINT64_C(78587730147417), UINT64_C(12843742829858255927)}, -68}
    }};
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b> constexpr std::array<std::int64_t, 5> atan_table_imp<b>::d32_coeffs;
template <bool b> constexpr std::array<std::int64_t, 12> atan_table_imp<b>::d64_coeffs;
template <bool b> constexpr std::array<int128::int128_t, 25> atan_table_imp<b>::d128_coeffs;
template <bool b> constexpr std::array<decimal32_t, 4> atan_table_imp<b>::d32_values;
template <bool b> constexpr std::array<decimal_fast32_t, 4> atan_table_imp<b>::d32_fast_values;
template <bool b> constexpr std::array<decimal64_t, 4> atan_table_imp<b>::d64_values;
template <bool b> constexpr std::array<decimal_fast64_t, 4> atan_table_imp<b>::d64_fast_values;
template <bool b> constexpr std::array<decimal128_t, 4> atan_table_imp<b>::d128_values;
template <bool b> constexpr std::array<decimal_fast128_t, 4> atan_table_imp<b>::d128_fast_values;

#endif

using atan_table = atan_table_imp<true>;

} //namespace atan_detail

// atan(x) - x = x^3 P(x^2)
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto atan_tail(T x) noexcept -> T;

template <> constexpr auto atan_tail<decimal32_t>(decimal32_t x) noexcept -> decimal32_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d32_coeffs); }
template <> constexpr auto atan_tail<decimal_fast32_t>(decimal_fast32_t x) noexcept -> decimal_fast32_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d32_coeffs); }
template <> constexpr auto atan_tail<decimal64_t>(decimal64_t x) noexcept -> decimal64_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d64_coeffs); }
template <> constexpr auto atan_tail<decimal_fast64_t>(decimal_fast64_t x) noexcept -> decimal_fast64_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d64_coeffs); }
template <> constexpr auto atan_tail<decimal128_t>(decimal128_t x) noexcept -> decimal128_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d128_coeffs); }
template <> constexpr auto atan_tail<decimal_fast128_t>(decimal_fast128_t x) noexcept -> decimal_fast128_t { return fixed_point_odd_tail(x, atan_detail::atan_table::d128_coeffs); }

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto atan_values(std::size_t idx) noexcept -> T;

template <> constexpr auto atan_values<decimal32_t>(std::size_t idx) noexcept -> decimal32_t { return atan_detail::atan_table::d32_values[idx]; }
template <> constexpr auto atan_values<decimal_fast32_t>(std::size_t idx) noexcept -> decimal_fast32_t { return atan_detail::atan_table::d32_fast_values[idx]; }
template <> constexpr auto atan_values<decimal64_t>(std::size_t idx) noexcept -> decimal64_t { return atan_detail::atan_table::d64_values[idx]; }
template <> constexpr auto atan_values<decimal_fast64_t>(std::size_t idx) noexcept -> decimal_fast64_t { return atan_detail::atan_table::d64_fast_values[idx]; }
template <> constexpr auto atan_values<decimal128_t>(std::size_t idx) noexcept -> decimal128_t { return atan_detail::atan_table::d128_values[idx]; }
template <> constexpr auto atan_values<decimal_fast128_t>(std::size_t idx) noexcept -> decimal_fast128_t { return atan_detail::atan_table::d128_fast_values[idx]; }

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto atan_series(T x) noexcept -> T
{
    return x + atan_tail(x);
}

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_IMPL_ATAN_IMPL_HPP
