// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SPLIT_PI_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SPLIT_PI_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/construction_sign.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

namespace split_pi_detail {

// Indices into split_pi_values: each _hi value is the constant rounded to the type and its
// _lo value is the part left over; three_quarter_pi has no low part, since only atan2 uses it
enum : std::size_t { quarter_pi_hi, quarter_pi_lo, half_pi_hi, half_pi_lo, three_quarter_pi, pi_hi, pi_lo };

// Use a struct of arrays so that we can have static constexpr arrays of coefficients.
// See https://github.com/boostorg/math/issues/923 for further information
template <bool b>
struct split_pi_table_imp
{
    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal32_t, 7> d32_values = {{
        decimal32_t {UINT64_C(7853982), -7},
        decimal32_t {UINT64_C(3660255), -14, construction_sign::negative},
        decimal32_t {UINT64_C(1570796), -6},
        decimal32_t {UINT64_C(3267949), -13},
        decimal32_t {UINT64_C(2356194), -6},
        decimal32_t {UINT64_C(3141593), -6},
        decimal32_t {UINT64_C(3464102), -13, construction_sign::negative}
    }};

    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal_fast32_t, 7> d32_fast_values = {{
        decimal_fast32_t {UINT64_C(7853982), -7},
        decimal_fast32_t {UINT64_C(3660255), -14, construction_sign::negative},
        decimal_fast32_t {UINT64_C(1570796), -6},
        decimal_fast32_t {UINT64_C(3267949), -13},
        decimal_fast32_t {UINT64_C(2356194), -6},
        decimal_fast32_t {UINT64_C(3141593), -6},
        decimal_fast32_t {UINT64_C(3464102), -13, construction_sign::negative}
    }};

    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal64_t, 7> d64_values = {{
        decimal64_t {UINT64_C(7853981633974483), -16},
        decimal64_t {UINT64_C(9615660845819876), -33},
        decimal64_t {UINT64_C(1570796326794897), -15},
        decimal64_t {UINT64_C(3807686783083602), -31, construction_sign::negative},
        decimal64_t {UINT64_C(2356194490192345), -15},
        decimal64_t {UINT64_C(3141592653589793), -15},
        decimal64_t {UINT64_C(2384626433832795), -31}
    }};

    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal_fast64_t, 7> d64_fast_values = {{
        decimal_fast64_t {UINT64_C(7853981633974483), -16},
        decimal_fast64_t {UINT64_C(9615660845819876), -33},
        decimal_fast64_t {UINT64_C(1570796326794897), -15},
        decimal_fast64_t {UINT64_C(3807686783083602), -31, construction_sign::negative},
        decimal_fast64_t {UINT64_C(2356194490192345), -15},
        decimal_fast64_t {UINT64_C(3141592653589793), -15},
        decimal_fast64_t {UINT64_C(2384626433832795), -31}
    }};

    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal128_t, 7> d128_values = {{
        decimal128_t {int128::uint128_t{UINT64_C(425765197510819), UINT64_C(5970600460659265253)}, -34},
        decimal128_t {int128::uint128_t{UINT64_C(114108442474915), UINT64_C(12088338259637095055)}, -68},
        decimal128_t {int128::uint128_t{UINT64_C(85153039502163), UINT64_C(15951515351099494343)}, -33},
        decimal128_t {int128::uint128_t{UINT64_C(239662122992084), UINT64_C(329523718765553795)}, -67},
        decimal128_t {int128::uint128_t{UINT64_C(127729559253245), UINT64_C(14703900989794465707)}, -33},
        decimal128_t {int128::uint128_t{UINT64_C(170306079004327), UINT64_C(13456286628489437071)}, -33},
        decimal128_t {int128::uint128_t{UINT64_C(62776840258584), UINT64_C(3343964766419005178)}, -67, construction_sign::negative}
    }};

    // pi/4, pi/2, 3 pi/4 and pi, each rounded to the type, and the rest of pi/4, pi/2 and pi
    static constexpr std::array<decimal_fast128_t, 7> d128_fast_values = {{
        decimal_fast128_t {int128::uint128_t{UINT64_C(425765197510819), UINT64_C(5970600460659265253)}, -34},
        decimal_fast128_t {int128::uint128_t{UINT64_C(114108442474915), UINT64_C(12088338259637095055)}, -68},
        decimal_fast128_t {int128::uint128_t{UINT64_C(85153039502163), UINT64_C(15951515351099494343)}, -33},
        decimal_fast128_t {int128::uint128_t{UINT64_C(239662122992084), UINT64_C(329523718765553795)}, -67},
        decimal_fast128_t {int128::uint128_t{UINT64_C(127729559253245), UINT64_C(14703900989794465707)}, -33},
        decimal_fast128_t {int128::uint128_t{UINT64_C(170306079004327), UINT64_C(13456286628489437071)}, -33},
        decimal_fast128_t {int128::uint128_t{UINT64_C(62776840258584), UINT64_C(3343964766419005178)}, -67, construction_sign::negative}
    }};
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b> constexpr std::array<decimal32_t, 7> split_pi_table_imp<b>::d32_values;
template <bool b> constexpr std::array<decimal_fast32_t, 7> split_pi_table_imp<b>::d32_fast_values;
template <bool b> constexpr std::array<decimal64_t, 7> split_pi_table_imp<b>::d64_values;
template <bool b> constexpr std::array<decimal_fast64_t, 7> split_pi_table_imp<b>::d64_fast_values;
template <bool b> constexpr std::array<decimal128_t, 7> split_pi_table_imp<b>::d128_values;
template <bool b> constexpr std::array<decimal_fast128_t, 7> split_pi_table_imp<b>::d128_fast_values;

#endif

using split_pi_table = split_pi_table_imp<true>;

} //namespace split_pi_detail

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto split_pi_values(std::size_t idx) noexcept -> T;

template <> constexpr auto split_pi_values<decimal32_t>(std::size_t idx) noexcept -> decimal32_t { return split_pi_detail::split_pi_table::d32_values[idx]; }
template <> constexpr auto split_pi_values<decimal_fast32_t>(std::size_t idx) noexcept -> decimal_fast32_t { return split_pi_detail::split_pi_table::d32_fast_values[idx]; }
template <> constexpr auto split_pi_values<decimal64_t>(std::size_t idx) noexcept -> decimal64_t { return split_pi_detail::split_pi_table::d64_values[idx]; }
template <> constexpr auto split_pi_values<decimal_fast64_t>(std::size_t idx) noexcept -> decimal_fast64_t { return split_pi_detail::split_pi_table::d64_fast_values[idx]; }
template <> constexpr auto split_pi_values<decimal128_t>(std::size_t idx) noexcept -> decimal128_t { return split_pi_detail::split_pi_table::d128_values[idx]; }
template <> constexpr auto split_pi_values<decimal_fast128_t>(std::size_t idx) noexcept -> decimal_fast128_t { return split_pi_detail::split_pi_table::d128_fast_values[idx]; }

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_IMPL_SPLIT_PI_HPP
