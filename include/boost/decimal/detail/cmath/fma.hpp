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

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Dec>
using components_type = std::conditional_t<std::is_same<Dec, decimal32_t>::value, decimal32_t_components,
                        std::conditional_t<std::is_same<Dec, decimal_fast32_t>::value, decimal_fast32_t_components,
                        std::conditional_t<std::is_same<Dec, decimal64_t>::value, decimal64_t_components,
                        std::conditional_t<std::is_same<Dec, decimal_fast64_t>::value, decimal_fast64_t_components,
                        std::conditional_t<std::is_same<Dec, decimal128_t>::value, decimal128_t_components, decimal_fast128_t_components
                        >>>>>;

template <bool checked, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto d32_fma_impl(T x, T y, T z) noexcept -> T
{
    using promoted_type = std::conditional_t<std::is_same<T, decimal32_t>::value, decimal64_t, decimal_fast64_t>;
    using promoted_components = components_type<promoted_type>;

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_DECIMAL_IF_CONSTEXPR (checked)
    {
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        {
            return x * y + z;
        }
    }
    #endif

    const auto x_components {x.to_components()};
    const auto y_components {y.to_components()};

    auto first_res {detail::mul_impl<promoted_components>(x_components, y_components)};

    // Apply the mul on the carried components
    // We still create the result as a decimal type to check for non-finite values and comparisons,
    // but we do not use it for the resultant calculation

    auto z_components {static_cast<promoted_components>(z.to_components())};

    detail::expand_significand<promoted_type>(z_components.sig, z_components.exp);
    detail::expand_significand<promoted_type>(first_res.sig, first_res.exp);

    return detail::add_impl<T>(first_res, z_components);
}

template <bool checked, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto d64_fma_impl(T x, T y, T z) noexcept -> T
{
    using promoted_type = std::conditional_t<std::is_same<T, decimal64_t>::value, decimal128_t, decimal_fast128_t>;
    using promoted_components = components_type<promoted_type>;

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_DECIMAL_IF_CONSTEXPR (checked)
    {
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        {
            return x * y + z;
        }
    }
    #endif

    const auto x_components {x.to_components()};
    const auto y_components {y.to_components()};

    auto first_res {detail::mul_impl<promoted_components>(x_components, y_components)};

    // Apply the mul on the carried components
    // We still create the result as a decimal type to check for non-finite values and comparisons,
    // but we do not use it for the resultant calculation

    auto z_components {static_cast<promoted_components>(z.to_components())};

    detail::expand_significand<promoted_type>(z_components.sig, z_components.exp);
    detail::expand_significand<promoted_type>(first_res.sig, first_res.exp);

    return detail::d128_add_impl_new<T>(first_res, z_components);
}

// 10^38 is the largest power of ten which 128 bits holds. Every divisor of the alignment
// stays at this width, thus u256 takes its narrow paths.
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::int32_t fma_narrow_digits {38};

// Marks a quotient which dropped a digit that is not zero: a last digit of zero becomes a
// one. The mark stays far below the last digit of the result, thus the rounding is exact.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_apply_sticky(const u256& quotient, const bool inexact) noexcept -> u256
{
    if (inexact && quotient % UINT64_C(10) == u256{0, 0, 0, 0})
    {
        return quotient + u256{0, 0, 0, 1};
    }

    return quotient;
}

// Divides by a power of ten and reports a dropped digit which is not zero. A shift of more
// than fma_narrow_digits goes in steps, because a narrow divisor costs less than a wide one.
BOOST_DECIMAL_CUDA_CONSTEXPR auto fma_shift_right(const u256& sig, std::int32_t shift, const std::int32_t width, bool& inexact) noexcept -> u256
{
    if (shift > width)
    {
        // Every digit falls below the sticky position, and the value is not zero.
        inexact = false;
        return u256{0, 0, 0, 1};
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

    // A zero in the product leaves one addition, and that already rounds one time.
    if (x_components.sig == 0U || y_components.sig == 0U)
    {
        return x * y + z;
    }

    auto z_components {z.to_components()};

    detail::expand_significand<T>(x_components.sig, x_components.exp);
    detail::expand_significand<T>(y_components.sig, y_components.exp);

    u256 product_sig {detail::umul256(x_components.sig, y_components.sig)};
    auto product_exp {static_cast<std::int32_t>(x_components.exp + y_components.exp)};
    const bool product_sign {x_components.sign != y_components.sign};

    if (z_components.sig == 0U)
    {
        const auto product_narrow {detail::fma_narrow_sum(product_sig, product_exp)};
        return T{product_narrow, product_exp, product_sign};
    }

    detail::expand_significand<T>(z_components.sig, z_components.exp);

    const auto addend_exp {static_cast<std::int32_t>(z_components.exp)};
    const bool addend_sign {z_components.sign};

    constexpr std::int32_t max_width {76};
    constexpr std::int32_t sig_dig {detail::precision_v<T>};
    constexpr std::int32_t product_width {2 * sig_dig};

    // The exponent of the sum. The product must not pass max_width digits on the way, and
    // the addend must not move by more than 38 digits.
    auto target_exp {product_exp < addend_exp ? product_exp : addend_exp};
    const std::int32_t product_floor {product_exp - (max_width - product_width)};
    const std::int32_t addend_floor {addend_exp - detail::fma_narrow_digits};

    if (target_exp < product_floor)
    {
        target_exp = product_floor;
    }
    if (target_exp < addend_floor)
    {
        target_exp = addend_floor;
    }

    // The multiplier goes as a u256. A u256 times a narrow integer reads the words of that
    // integer from its memory, and that order is wrong on a big-endian machine.
    if (product_exp > target_exp)
    {
        product_sig = product_sig * detail::pow10(u256{static_cast<std::uint64_t>(product_exp - target_exp)});
    }
    else if (product_exp < target_exp)
    {
        bool inexact {};
        const auto shifted {detail::fma_shift_right(product_sig, target_exp - product_exp, product_width, inexact)};
        product_sig = detail::fma_apply_sticky(shifted, inexact);
    }

    // The addend is still 128 bits here. A move down is one multiply into 256 bits, and a
    // move up is one division of 128 bits.
    u256 addend_sig {};

    if (addend_exp > target_exp)
    {
        addend_sig = detail::umul256(z_components.sig,
                                     detail::pow10_u128(static_cast<std::size_t>(addend_exp - target_exp)));
    }
    else if (addend_exp == target_exp)
    {
        addend_sig = u256{z_components.sig};
    }
    else if (target_exp - addend_exp >= sig_dig)
    {
        // Every digit of the addend falls below the sticky position.
        addend_sig = u256{0, 0, 0, 1};
    }
    else
    {
        const auto divisor {detail::pow10_u128(static_cast<std::size_t>(target_exp - addend_exp))};
        const auto quotient {z_components.sig / divisor};
        addend_sig = detail::fma_apply_sticky(u256{quotient}, quotient * divisor != z_components.sig);
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
        // IEEE 754-2019 6.3 asks for a negative zero in the downward mode. The operators and
        // the 32 and 64 bit paths give a positive zero in every mode, thus this path does too.
        result_sign = false;
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
