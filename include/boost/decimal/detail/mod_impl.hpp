// Copyright 2023 - 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/add_impl.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#include <limits>
#include <type_traits>
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant (pre-C++17 if-constexpr fallback)
#  pragma warning(disable : 4324) // Structure was padded due to alignment specifier
#endif

namespace boost {
namespace decimal {
namespace detail {

template <typename T>
constexpr auto get_pow_10(const std::size_t value) noexcept -> T
{
    static_assert(std::is_same<T, std::uint64_t>::value, "Should not be used");
    return static_cast<T>(value);
}

template <>
constexpr auto get_pow_10(const std::size_t value) noexcept -> std::uint64_t
{
    return pow10<std::uint64_t>(value);
}

template <>
constexpr auto get_pow_10(const std::size_t value) noexcept -> int128::uint128_t
{
    return pow10_u128(value);
}

// lhs * rhs in Promoted, which holds the full product of the operands used
template <typename Promoted, typename Small>
constexpr auto wide_mul(const Small lhs, const Small rhs) noexcept -> Promoted
{
    return Promoted {lhs} * rhs;
}

template <>
constexpr auto wide_mul<u256, int128::uint128_t>(const int128::uint128_t lhs, const int128::uint128_t rhs) noexcept -> u256
{
    return umul256(lhs, rhs);
}

// (r * p) mod m. The specializations below need r < m and p < 2^64.
template <typename Promoted, typename Small>
constexpr auto mul_mod(const Small r, const Small p, const Small m) noexcept -> Small
{
    return static_cast<Small>(wide_mul<Promoted>(r, p) % m);
}

template <>
constexpr auto mul_mod<int128::uint128_t, std::uint64_t>(const std::uint64_t r, const std::uint64_t p, const std::uint64_t m) noexcept -> std::uint64_t
{
    // r < m keeps the high word of the product below m, so one division is enough
    std::uint64_t high {};
    const auto low {int128::detail::umul(r, p, high)};
    std::uint64_t rem {};
    static_cast<void>(int128::detail::udiv_2by1(high, low, m, rem));
    return rem;
}

// The remainder of the three words (w2, w1, w0) by m >= 2^64, for (w2, w1) < m
constexpr auto rem_3by2(const std::uint64_t w2, const std::uint64_t w1, const std::uint64_t w0, const int128::uint128_t m) noexcept -> int128::uint128_t
{
    // Shift the value and m so the top bit of m is set
    const auto s {int128::detail::countl_zero(m.high)};
    const auto d {m << s};
    const auto u0 {w0 << s};
    const auto u1 {s == 0 ? w1 : (w1 << s) | (w0 >> (64 - s))};
    const auto u2 {s == 0 ? w2 : (w2 << s) | (w1 >> (64 - s))};

    // The estimate from the top words is at most 2 too large (Knuth, theorem B)
    std::uint64_t q {UINT64_MAX};
    if (u2 < d.high)
    {
        std::uint64_t unused {};
        q = int128::detail::udiv_2by1(u2, u1, d.high, unused);
    }

    std::uint64_t t2 {};
    const auto t_mid {int128::detail::umul(q, d.high, t2)};
    std::uint64_t t_carry {};
    const auto t0 {int128::detail::umul(q, d.low, t_carry)};
    const auto t1 {t_mid + t_carry};
    t2 += static_cast<std::uint64_t>(t1 < t_mid);

    const int128::uint128_t u_low {u1, u0};
    const int128::uint128_t t_low {t1, t0};
    auto rem {u_low - t_low};
    auto rem_high {u2 - t2 - static_cast<std::uint64_t>(u_low < t_low)};

    // A negative difference has a nonzero top word. Add d back into [0, d).
    while (rem_high != 0U)
    {
        rem += d;
        rem_high += static_cast<std::uint64_t>(rem < d);
    }

    return rem >> s;
}

template <>
constexpr auto mul_mod<u256, int128::uint128_t>(const int128::uint128_t r, const int128::uint128_t p, const int128::uint128_t m) noexcept -> int128::uint128_t
{
    if (m.high == 0U)
    {
        return int128::uint128_t {0U, mul_mod<int128::uint128_t, std::uint64_t>(r.low, p.low, m.low)};
    }

    // r * p < m * 2^64 has three words
    std::uint64_t carry_low {};
    const auto w0 {int128::detail::umul(r.low, p.low, carry_low)};
    std::uint64_t w2 {};
    const auto mid {int128::detail::umul(r.high, p.low, w2)};
    const auto w1 {mid + carry_low};
    w2 += static_cast<std::uint64_t>(w1 < mid);
    return rem_3by2(w2, w1, w0, m);
}

// (a * b) mod m for a < m, and b < m or b < 2^64
constexpr auto mul_mod_any(const std::uint64_t a, const std::uint64_t b, const std::uint64_t m) noexcept -> std::uint64_t
{
    return mul_mod<int128::uint128_t>(a, b, m);
}

constexpr auto mul_mod_any(const int128::uint128_t a, const int128::uint128_t b, const int128::uint128_t m) noexcept -> int128::uint128_t
{
    if (m.high == 0U || b.high == 0U)
    {
        return mul_mod<u256>(a, b, m);
    }

    // Reduce the top three of the four words, then the rest
    const auto product {umul256(a, b)};
    const auto top {rem_3by2(product[3], product[2], product[1], m)};
    return rem_3by2(top.high, top.low, product[0], m);
}

// (r * (10^19)^n) mod m for r < m
template <typename Small>
constexpr auto mul_pow_mod(Small r, int n, const Small m) noexcept -> Small
{
    auto power {mul_mod_any(Small {1U}, Small {UINT64_C(10000000000000000000)}, m)};
    for (;;)
    {
        if ((n & 1) != 0)
        {
            r = mul_mod_any(r, power, m);
        }
        n >>= 1;
        if (n == 0)
        {
            return r;
        }
        power = mul_mod_any(power, power, m);
    }
}

// The divisor stays below 2 * 10^p, so 16 times it fits this type
template <typename DecimalType>
using mod_small_t = std::conditional_t<decimal_val_v<DecimalType> < 128, std::uint64_t, int128::uint128_t>;

template <typename Small>
struct exact_mod_result
{
    Small coeff;        // The magnitude of the result is coeff * 10^exp
    int exp;
    unsigned quotient;  // The low bits of the quotient
    bool flip_sign;     // The result has the sign of -x
};

// Exact |x| mod |y| at exponent min(ex, ey). A QuotientModulus above 1
// also rounds the quotient half to even (remainder, remquo).
template <typename DecimalType, unsigned QuotientModulus, typename Significand>
constexpr auto exact_mod(const Significand mx, const int ex, const Significand my, const int ey) noexcept
    -> exact_mod_result<mod_small_t<DecimalType>>
{
    static_assert(QuotientModulus <= 16U && (QuotientModulus & (QuotientModulus - 1U)) == 0U,
                  "The quotient modulus is a power of two up to 16");

    using promoted_type = std::conditional_t<decimal_val_v<DecimalType> < 64, std::uint64_t,
                              std::conditional_t<decimal_val_v<DecimalType> < 128, int128::uint128_t, u256>>;
    using small_type = mod_small_t<DecimalType>;

    BOOST_DECIMAL_ASSERT(my != 0U);

    constexpr int digits {std::numeric_limits<DecimalType>::digits10};
    exact_mod_result<small_type> result {static_cast<small_type>(mx), ex, 0U, false};
    small_type divisor {};

    // A zero x is its own remainder
    if (mx == 0U)
    {
        result.exp = ex < ey ? ex : ey;
        return result;
    }

    if (ey > ex)
    {
        // |x| < 10^(ex + p) <= 10^(ey - 1) < |y| / 2, so x is its own remainder
        if (ey - ex > digits)
        {
            return result;
        }

        const auto scaled_y {wide_mul<promoted_type>(static_cast<small_type>(my), get_pow_10<small_type>(static_cast<std::size_t>(ey - ex)))};

        // |y| > 2|x| also leaves x. Else |y| < 2 * 10^p at the exponent of x.
        const small_type twice_x {result.coeff * 2U};
        if (scaled_y > promoted_type {twice_x})
        {
            return result;
        }

        divisor = static_cast<small_type>(scaled_y);
        result.coeff %= divisor * QuotientModulus;
    }
    else
    {
        // Reduce the gap in steps of at most 10^19. Zero stays zero.
        constexpr int max_step {std::numeric_limits<std::uint64_t>::digits10};
        using step_type = std::conditional_t<decimal_val_v<DecimalType> < 128, int128::uint128_t, u256>;

        divisor = static_cast<small_type>(my);
        const small_type modulus {divisor * QuotientModulus};
        int gap {ex - ey};

        // mx * 10^(19 - p) fits 64 bits for decimal32_t, so no reduction first
        BOOST_DECIMAL_IF_CONSTEXPR (decimal_val_v<DecimalType> < 64)
        {
            constexpr int first_max {max_step - digits};
            const int first {gap < first_max ? gap : first_max};
            result.coeff = result.coeff * get_pow_10<small_type>(static_cast<std::size_t>(first)) % modulus;
            gap -= first;
        }
        else
        {
            result.coeff %= modulus;
        }

        // From four steps up, squaring gives (10^19)^n mod m in log2(n) steps
        if (gap >= 4 * max_step && result.coeff != 0U)
        {
            result.coeff = mul_pow_mod(result.coeff, gap / max_step, modulus);
            gap %= max_step;
        }

        for (; gap > 0 && result.coeff != 0U; gap -= max_step)
        {
            const auto power {get_pow_10<small_type>(static_cast<std::size_t>(gap < max_step ? gap : max_step))};
            result.coeff = mul_mod<step_type>(result.coeff, power, modulus);
        }

        result.exp = ey;
    }

    // fmod is below y already. Else take the low quotient bits and round half
    // to even, which gives y minus the remainder with the other sign.
    BOOST_DECIMAL_IF_CONSTEXPR (QuotientModulus > 1U)
    {
        while (result.coeff >= divisor)
        {
            result.coeff -= divisor;
            ++result.quotient;
        }

        const small_type twice_remainder {result.coeff * 2U};

        if (twice_remainder > divisor || (twice_remainder == divisor && (result.quotient & 1U) == 1U))
        {
            result.coeff = divisor - result.coeff;
            ++result.quotient;
            result.flip_sign = true;
        }
    }

    return result;
}

// The result of a modulo by zero: NaN, or zero with fast math
template <typename DecimalType>
constexpr auto mod_by_zero() noexcept -> DecimalType
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    return std::numeric_limits<DecimalType>::quiet_NaN();
    #else
    return DecimalType {0, 0};
    #endif
}

template <typename DecimalType, typename ComponentsType>
constexpr auto generic_mod_impl(const ComponentsType& lhs_components, const ComponentsType& rhs_components) noexcept
    BOOST_DECIMAL_REQUIRES_TWO_RETURN(is_decimal_floating_point_v, DecimalType, is_decimal_floating_point_components_v, ComponentsType, DecimalType)
{
    if (rhs_components.sig == 0U)
    {
        return mod_by_zero<DecimalType>();
    }

    const auto result {exact_mod<DecimalType, 1U>(lhs_components.sig, lhs_components.exp, rhs_components.sig, rhs_components.exp)};
    return pack_in_range<DecimalType>(result.coeff, result.exp, lhs_components.sign);
}

} // namespace detail
} // namespace decimal
} // namespace boost

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif // BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP
