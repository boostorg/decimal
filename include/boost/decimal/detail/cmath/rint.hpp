// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/impl/round_integral.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#include <cmath>
#include <climits>
#endif

namespace boost {
namespace decimal {

namespace detail {

// The current rounding mode
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto current_rounding_mode(const T x) noexcept -> rounding_mode
{
    static_cast<void>(x);
    auto round {_boost_decimal_global_rounding_mode};
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    if (!BOOST_DECIMAL_IS_CONSTANT_EVALUATED(x))
    {
        round = _boost_decimal_global_runtime_rounding_mode;
    }
    #endif

    return round;
}

template <BOOST_DECIMAL_INTEGRAL Int>
constexpr auto lrint_saturate(const bool sign) noexcept -> Int
{
    return sign ? (std::numeric_limits<Int>::min)() : (std::numeric_limits<Int>::max)();
}

// MSVC 14.1 warns of unary minus being applied to unsigned type from numeric_limits::min
// 14.2 and on get it right
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4146)
#endif

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T, BOOST_DECIMAL_INTEGRAL Int>
constexpr auto lrint_impl(const T num) noexcept -> Int
{
    using sig_type = typename T::significand_type;
    using uint_type = std::make_unsigned_t<Int>;
    using work_type = std::conditional_t<(sizeof(sig_type) > sizeof(uint_type)), sig_type, uint_type>;

    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(num))
    {
        // Implementation defined what to return here
        return std::numeric_limits<Int>::min();
    }
    #endif

    const auto c {decompose(num)};
    if (c.sig == 0U)
    {
        return 0;
    }

    // Largest magnitude of the result: max for x > 0, and -min for x < 0
    const auto limit {static_cast<work_type>(static_cast<uint_type>((std::numeric_limits<Int>::max)()) + (c.sign ? 1U : 0U))};
    work_type mag {};
    if (c.exp >= 0)
    {
        // 10^exp overflows work_type: saturate
        if (c.exp > std::numeric_limits<uint_type>::digits10)
        {
            return lrint_saturate<Int>(c.sign);
        }
        const auto p {pow10(static_cast<work_type>(c.exp))};
        if (static_cast<work_type>(c.sig) > limit / p)
        {
            return lrint_saturate<Int>(c.sign);
        }
        mag = static_cast<work_type>(static_cast<work_type>(c.sig) * p);
    }
    else
    {
        // |x| >= 10^(digits10 + 1) is out of range: saturate before the division
        constexpr int int_digits {std::numeric_limits<uint_type>::digits10 + 1};
        const auto shift {-static_cast<int>(c.exp)};
        if (shift < precision_v<T> - int_digits && c.sig >= pow10(static_cast<sig_type>(shift + int_digits)))
        {
            return lrint_saturate<Int>(c.sign);
        }
        mag = static_cast<work_type>(round_integral_sig<T>(c, current_rounding_mode(num)));
    }

    if (mag > limit)
    {
        return lrint_saturate<Int>(c.sign);
    }

    const auto umag {static_cast<uint_type>(mag)};
    return static_cast<Int>(c.sign ? static_cast<uint_type>(0U - umag) : umag);
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

} //namespace detail

// Rounds the number using the default rounding mode
BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto rint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    return detail::round_integral_impl(num, detail::current_rounding_mode(num));
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto lrint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long)
{
    return detail::lrint_impl<T, long>(num);
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto llrint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long long)
{
    return detail::lrint_impl<T, long long>(num);
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP
