// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_FMA_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_FMA_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/i256.hpp>
#include <boost/decimal/detail/normalize.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/u256.hpp>

namespace boost {
namespace decimal {

namespace detail {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

// A division by ten is slow for the wide types. 2^64 is 1 modulo 5, thus the sum of the
// words is the value modulo 5.
constexpr auto fma_last_digit_is_zero(const std::uint64_t value) noexcept -> bool
{
    return value % 10U == 0U;
}

constexpr auto fma_last_digit_is_zero(const int128::uint128_t& value) noexcept -> bool
{
    return (value.low & 1U) == 0U && (value.low % 5U + value.high % 5U) % 5U == 0U;
}

BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_last_digit_is_zero(const u256& value) noexcept -> bool
{
    return (value[0] & 1U) == 0U && (value[0] % 5U + value[1] % 5U + value[2] % 5U + value[3] % 5U) % 5U == 0U;
}

// A sum of 2p - 1 or 2p digits rounds like a product, in one step.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto fma_finalize(const std::uint64_t sig, const std::int32_t exp, const bool sign) noexcept -> T
{
    if (BOOST_DECIMAL_UNLIKELY(!impl::mul_default_rounding(sig)))
    {
        return impl::mul_finalize_u64_out_of_line<T>(sig, exp, sign, detail::current_rounding_mode());
    }

    return impl::mul_finalize_u64<T>(sig, exp, sign);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto fma_finalize(const int128::uint128_t& sig, const std::int32_t exp, const bool sign) noexcept -> T
{
    if (BOOST_DECIMAL_UNLIKELY(!impl::mul_default_rounding(sig)))
    {
        return impl::mul_finalize_u128_out_of_line<T>(sig, exp, sign, detail::current_rounding_mode());
    }

    return impl::mul_finalize_u128<T>(sig, exp, sign);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto fma_finalize(const u256& sig, const std::int32_t exp, const bool sign) noexcept -> T
{
    if (BOOST_DECIMAL_UNLIKELY(!impl::mul_default_rounding(sig)))
    {
        return impl::mul_finalize_u256_out_of_line<T>(sig, exp, sign, detail::current_rounding_mode());
    }

    return impl::mul_finalize_u256<T>(sig, exp, sign);
}

// The product of two significands of p digits fits in Wide. The sum is exact but for one
// sticky digit far below the last digit of the result, thus it rounds one time.
template <typename Wide, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T, typename Components>
BOOST_DECIMAL_FORCE_INLINE constexpr auto narrow_fma_impl(T x, T y, T z, Components x_components, Components y_components, Components z_components) noexcept -> T
{
    // A zero operand leaves one operation, and that already rounds one time.
    if (x_components.sig == 0U || y_components.sig == 0U)
    {
        return x * y + z;
    }
    if (z_components.sig == 0U)
    {
        return x * y;
    }

    detail::expand_significand<T>(x_components.sig, x_components.exp);
    detail::expand_significand<T>(y_components.sig, y_components.exp);
    detail::expand_significand<T>(z_components.sig, z_components.exp);

    constexpr std::int32_t sig_dig {detail::precision_v<T>};

    Wide product {static_cast<Wide>(x_components.sig) * static_cast<Wide>(y_components.sig)};
    const auto product_exp {static_cast<std::int32_t>(x_components.exp + y_components.exp)};
    const bool product_sign {x_components.sign != y_components.sign};
    const auto addend_exp {static_cast<std::int32_t>(z_components.exp)};

    // The product does not move up and a far addend moves up p digits, thus a sum of 2p - 1 or 2p
    // digits is common. Only an operand 10 times smaller drops digits, and inexact records them.
    Wide addend {z_components.sig};
    std::int32_t target_exp {product_exp};
    bool inexact {};
    bool inexact_sign {z_components.sign};

    if (addend_exp >= product_exp)
    {
        const auto gap {addend_exp - product_exp};
        if (gap <= sig_dig + 1)
        {
            addend *= detail::pow10(static_cast<Wide>(gap));
        }
        else
        {
            target_exp = addend_exp - sig_dig;
            addend *= detail::pow10(static_cast<Wide>(sig_dig));
            inexact_sign = product_sign;
            const auto shift {target_exp - product_exp};
            if (shift >= 2 * sig_dig)
            {
                product = 0U;
                inexact = true;
            }
            else
            {
                const auto divisor {detail::pow10(static_cast<Wide>(shift))};
                const auto quotient {product / divisor};
                inexact = quotient * divisor != product;
                product = quotient;
            }
        }
    }
    else
    {
        const auto shift {product_exp - addend_exp};
        if (shift >= sig_dig)
        {
            addend = 0U;
            inexact = true;
        }
        else
        {
            const auto divisor {detail::pow10(static_cast<typename Components::significand_type>(shift))};
            const auto quotient {z_components.sig / divisor};
            inexact = quotient * divisor != z_components.sig;
            addend = quotient;
        }
    }

    Wide result {};
    bool result_sign {product_sign};

    if (product_sign == z_components.sign)
    {
        result = product + addend;
    }
    else if (product >= addend)
    {
        result = product - addend;
        if (result == 0U)
        {
            result_sign = detail::cancel_sign(x);
        }
    }
    else
    {
        result = addend - product;
        result_sign = z_components.sign;
    }

    // The exact sum is between result and the next integer toward the dropped digits. The one
    // of the two with a last digit that is not zero rounds the same, thus it holds the sticky.
    if (inexact)
    {
        if (inexact_sign != result_sign)
        {
            --result;
        }
        if (fma_last_digit_is_zero(result))
        {
            ++result;
        }
    }

    // If the rounded result falls below the smallest exponent, the constructor must round,
    // because a second rounding there is not exact.
    constexpr std::int32_t finalize_floor {detail::etiny_v<T> - sig_dig + 1};
    if (result >= detail::pow10(static_cast<Wide>(2 * sig_dig - 2)) && result < detail::pow10(static_cast<Wide>(2 * sig_dig)) && target_exp >= finalize_floor)
    {
        return fma_finalize<T>(result, target_exp, result_sign);
    }

    return T{result, target_exp, result_sign};
}

template <bool checked, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto d32_fma_impl(T x, T y, T z) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_DECIMAL_IF_CONSTEXPR (checked)
    {
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        {
            return x * y + z;
        }
    }
    #endif

    return narrow_fma_impl<std::uint64_t>(x, y, z, x.to_components(), y.to_components(), z.to_components());
}

template <bool checked, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto d64_fma_impl(T x, T y, T z) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_DECIMAL_IF_CONSTEXPR (checked)
    {
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        {
            return x * y + z;
        }
    }
    #endif

    return narrow_fma_impl<int128::uint128_t>(x, y, z, x.to_components(), y.to_components(), z.to_components());
}

// 10^38 is the largest power of ten which 128 bits holds. Every divisor of the alignment
// stays at this width, thus u256 takes its narrow paths.
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::int32_t fma_narrow_digits {38};

// The product of a significand below 10^34 < 2^113 and a factor up to 10^38. The significand
// has at most 113 bits, thus the middle sum does not carry out of 128 bits.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_umul256(const int128::uint128_t& sig, const int128::uint128_t& factor) noexcept -> u256
{
    const auto low {int128::uint128_t{sig.low} * factor.low};
    const auto middle {int128::uint128_t{sig.low} * factor.high + int128::uint128_t{sig.high} * factor.low + int128::uint128_t{low.high}};
    const auto high {int128::uint128_t{sig.high} * factor.high + int128::uint128_t{middle.high}};

    return u256{high.high, high.low, middle.low, low.low};
}

// Marks a value which dropped a digit that is not zero: a last digit of zero becomes a one.
// The mark stays far below the last digit of the result, thus the rounding is exact.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_apply_sticky(const u256& value) noexcept -> u256
{
    if (fma_last_digit_is_zero(value))
    {
        return value + u256{0, 0, 0, 1};
    }

    return value;
}

// Divides by a power of ten and reports a dropped digit which is not zero. A shift of more
// than fma_narrow_digits goes in steps, because a narrow divisor costs less than a wide one.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_shift_right(const u256& sig, std::int32_t shift, const std::int32_t width, bool& inexact) noexcept -> u256
{
    if (shift > width)
    {
        // Every digit falls off, and the value is not zero.
        inexact = true;
        return u256{0, 0, 0, 0};
    }

    u256 quotient {sig};
    inexact = false;

    while (shift > 0)
    {
        const auto step {shift < fma_narrow_digits ? shift : fma_narrow_digits};
        const auto result {detail::impl::div_mod(quotient, detail::pow10_u128(static_cast<std::size_t>(step)))};

        inexact = inexact || result.remainder != u256{0, 0, 0, 0};
        quotient = result.quotient;
        shift -= step;
    }

    return quotient;
}

// Narrows the sum to the 38 digits of a 128 bit significand, with the sticky mark in the
// last place. The constructor then rounds those digits one time.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_narrow_sum(const u256& sig, std::int32_t& exp) noexcept -> int128::uint128_t
{
    const std::int32_t digits {detail::num_digits(sig)};

    if (digits <= fma_narrow_digits)
    {
        return int128::uint128_t{sig[1], sig[0]};
    }

    const auto shift {digits - fma_narrow_digits};
    bool inexact {};
    const auto shifted {fma_shift_right(sig, shift, digits, inexact)};
    exp += shift;

    // The quotient holds 38 digits, thus the mark goes on in 128 bits and not in 256.
    int128::uint128_t narrow {shifted[1], shifted[0]};

    if (inexact && narrow % UINT64_C(10) == 0U)
    {
        ++narrow;
    }

    return narrow;
}

// fma must calculate x * y + z with one rounding. This path holds the exact product of the
// two significands in 256 bits, it aligns the addend to it, and it rounds one time.
template <bool checked, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto d128_fma_impl(T x, T y, T z) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_DECIMAL_IF_CONSTEXPR (checked)
    {
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        {
            return x * y + z;
        }
    }
    #endif

    auto x_components {x.to_components()};
    auto y_components {y.to_components()};
    auto z_components {z.to_components()};

    // A zero operand leaves one operation, and that already rounds one time.
    if (x_components.sig == 0U || y_components.sig == 0U)
    {
        return x * y + z;
    }
    if (z_components.sig == 0U)
    {
        return x * y;
    }

    detail::expand_significand<T>(x_components.sig, x_components.exp);
    detail::expand_significand<T>(y_components.sig, y_components.exp);

    u256 product_sig {detail::fma_umul256(x_components.sig, y_components.sig)};
    const auto product_exp {static_cast<std::int32_t>(x_components.exp + y_components.exp)};
    const bool product_sign {x_components.sign != y_components.sign};

    detail::expand_significand<T>(z_components.sig, z_components.exp);

    const auto addend_exp {static_cast<std::int32_t>(z_components.exp)};
    const bool addend_sign {z_components.sign};

    constexpr std::int32_t sig_dig {detail::precision_v<T>};
    constexpr std::int32_t product_width {2 * sig_dig};

    // The exponent of the sum. The product does not move up and a far addend moves up 34 digits,
    // thus a sum of 67 or 68 digits is common. Only an operand 10 times smaller drops digits.
    auto target_exp {product_exp};
    bool inexact {};
    bool inexact_sign {addend_sign};

    if (target_exp < addend_exp - sig_dig - 1)
    {
        target_exp = addend_exp - sig_dig;
        inexact_sign = product_sign;
        product_sig = detail::fma_shift_right(product_sig, target_exp - product_exp, product_width, inexact);
    }

    // The addend is still 128 bits here. If addend_exp is above target_exp, it takes one
    // multiply into 256 bits, else one division of 128 bits.
    u256 addend_sig {};

    if (addend_exp > target_exp)
    {
        addend_sig = detail::fma_umul256(z_components.sig,
                                         detail::pow10_u128(static_cast<std::size_t>(addend_exp - target_exp)));
    }
    else if (addend_exp == target_exp)
    {
        addend_sig = u256{z_components.sig};
    }
    else if (target_exp - addend_exp >= sig_dig)
    {
        // Every digit of the addend falls off.
        inexact = true;
    }
    else
    {
        const auto result {detail::impl::divmod_pow10_uint128(z_components.sig, target_exp - addend_exp)};
        addend_sig = u256{result.quotient};
        inexact = result.remainder != 0U;
    }

    u256 result_sig {};
    bool result_sign {};

    if (product_sign == addend_sign)
    {
        result_sig = product_sig + addend_sig;
        result_sign = product_sign;
    }
    else
    {
        // i256_sub gives the magnitude, and it reports which operand was the larger one.
        const auto product_is_smaller {detail::i256_sub(product_sig, addend_sig, result_sig)};
        result_sign = product_is_smaller ? addend_sign : product_sign;
    }

    if (result_sig == u256{0, 0, 0, 0})
    {
        result_sign = detail::cancel_sign(x);
    }

    // The exact sum is between result_sig and the next integer toward the dropped digits. The
    // one of the two with a last digit that is not zero rounds the same, thus it holds the sticky.
    if (inexact)
    {
        if (inexact_sign != result_sign)
        {
            result_sig = result_sig - u256{0, 0, 0, 1};
        }
        result_sig = detail::fma_apply_sticky(result_sig);
    }

    // A sum of 67 or 68 digits rounds like a product, in one step. Below the smallest exponent
    // the constructor must round instead, because a second rounding there is not exact.
    constexpr std::int32_t finalize_floor {detail::etiny_v<T> - sig_dig + 1};
    if (result_sig >= detail::pow10(u256{static_cast<std::uint64_t>(product_width - 2)}) && result_sig < detail::pow10(u256{static_cast<std::uint64_t>(product_width)}) && target_exp >= finalize_floor)
    {
        return fma_finalize<T>(result_sig, target_exp, result_sign);
    }

    const auto result_sig_narrow {detail::fma_narrow_sum(result_sig, target_exp)};

    return T{result_sig_narrow, target_exp, result_sign};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

constexpr auto unchecked_fma(const decimal32_t x, const decimal32_t y, const decimal32_t z) noexcept -> decimal32_t
{
    return detail::d32_fma_impl<false>(x, y, z);
}

constexpr auto unchecked_fma(const decimal_fast32_t x, const decimal_fast32_t y, const decimal_fast32_t z) noexcept -> decimal_fast32_t
{
    return detail::d32_fma_impl<false>(x, y, z);
}

constexpr auto unchecked_fma(const decimal64_t x, const decimal64_t y, const decimal64_t z) noexcept -> decimal64_t
{
    return detail::d64_fma_impl<false>(x, y, z);
}

constexpr auto unchecked_fma(const decimal_fast64_t x, const decimal_fast64_t y, const decimal_fast64_t z) noexcept -> decimal_fast64_t
{
    return detail::d64_fma_impl<false>(x, y, z);
}

constexpr auto unchecked_fma(const decimal128_t x, const decimal128_t y, const decimal128_t z) noexcept -> decimal128_t
{
    return detail::d128_fma_impl<false>(x, y, z);
}

constexpr auto unchecked_fma(const decimal_fast128_t x, const decimal_fast128_t y, const decimal_fast128_t z) noexcept -> decimal_fast128_t
{
    return detail::d128_fma_impl<false>(x, y, z);
}

} // Namespace detail

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal32_t x, const decimal32_t y, const decimal32_t z) noexcept -> decimal32_t
{
    return detail::d32_fma_impl<true>(x, y, z);
}

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal64_t x, const decimal64_t y, const decimal64_t z) noexcept -> decimal64_t
{
    return detail::d64_fma_impl<true>(x, y, z);
}

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal128_t x, const decimal128_t y, const decimal128_t z) noexcept -> decimal128_t
{
    return detail::d128_fma_impl<true>(x, y, z);
}

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal_fast32_t x, const decimal_fast32_t y, const decimal_fast32_t z) noexcept -> decimal_fast32_t
{
    return detail::d32_fma_impl<true>(x, y, z);
}

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal_fast64_t x, const decimal_fast64_t y, const decimal_fast64_t z) noexcept -> decimal_fast64_t
{
    return detail::d64_fma_impl<true>(x, y, z);
}

BOOST_DECIMAL_EXPORT constexpr auto fma(const decimal_fast128_t x, const decimal_fast128_t y, const decimal_fast128_t z) noexcept -> decimal_fast128_t
{
    return detail::d128_fma_impl<true>(x, y, z);
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_IMPL_FMA_HPP
