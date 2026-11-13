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

template <> constexpr auto atan_tail<decimal32_t>(decimal32_t x) noexcept -> decimal32_t { return fixed_point_odd_tail(x, fx_table::atan_d32); }
template <> constexpr auto atan_tail<decimal_fast32_t>(decimal_fast32_t x) noexcept -> decimal_fast32_t { return fixed_point_odd_tail(x, fx_table::atan_d32); }
template <> constexpr auto atan_tail<decimal64_t>(decimal64_t x) noexcept -> decimal64_t { return fixed_point_odd_tail(x, fx_table::atan_d64); }
template <> constexpr auto atan_tail<decimal_fast64_t>(decimal_fast64_t x) noexcept -> decimal_fast64_t { return fixed_point_odd_tail(x, fx_table::atan_d64); }
template <> constexpr auto atan_tail<decimal128_t>(decimal128_t x) noexcept -> decimal128_t { return fixed_point_odd_tail(x, fx_table::atan_d128); }
template <> constexpr auto atan_tail<decimal_fast128_t>(decimal_fast128_t x) noexcept -> decimal_fast128_t { return fixed_point_odd_tail(x, fx_table::atan_d128); }

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
