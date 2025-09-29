// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
#define BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP

#if __has_include(<decimal/decimal>) || defined(BOOST_DECIMAL_HAS_BUILTIN_DECIMAL)

#ifndef BOOST_DECIMAL_HAS_BUILTIN_DECIMAL
#  define BOOST_DECIMAL_HAS_BUILTIN_DECIMAL
#endif

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/dpd_conversion.hpp>
#include <boost/decimal/bid_conversion.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE

#include <decimal/decimal>

#endif

// On platforms such as POWER and zSystem the user could have a hardware decimal floating point unit
// This class serves as the wrapper that we then use to specialize support for the rest of the lib.
// We can also wrap GCC's BID type in the event that hardware type doesn't exist,
// which still allows users to utilize that type with the entire standard library that we provide here

namespace boost {
namespace decimal {
namespace detail {

#if defined(__s390x__) || (defined(__PPC64__) || defined(__powerpc64__))

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE bool _builtin_decimal_is_dpd {true};

#else

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE bool _builtin_decimal_is_dpd {false};

#endif // Architectures with DPD

template <typename BasisType>
class hardware_wrapper
{
private:

    static_assert(sizeof(BasisType) == sizeof(decimal32_t) ||
                  sizeof(BasisType) == sizeof(decimal64_t) ||
                  sizeof(BasisType) == sizeof(decimal128_t), "Only _Decimal32, _Decimal64, and _Decimal128 are supported");

    using bid_type = std::conditional_t<(sizeof(BasisType) <= sizeof(decimal32_t)), decimal32_t,
                        std::conditional_t<(sizeof(BasisType) <= sizeof(decimal64_t)), decimal64_t, decimal128_t>>;

public:

    using significand_type = typename bid_type::significand_type;
    using exponent_type = typename bid_type::exponent_type;
    using biased_exponent_type = typename bid_type::biased_exponent_type;

private:

    BasisType basis_ {};

public:

    explicit hardware_wrapper(const BasisType value) : basis_(value) {}

    template <typename T>
    explicit hardware_wrapper(const T value) : basis_(value) {}

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_UNSIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(T1 coeff, T2 exp, bool sign = false) noexcept
    {
        const bid_type bid_value {coeff, exp, sign};
        const auto dpd_bits {to_dpd(bid_value)};
        std::memcpy(&basis_, &dpd_bits, sizeof(decltype(dpd_bits)));
    }

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_SIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<!detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(T1 coeff, T2 exp) noexcept : hardware_wrapper(detail::make_positive_unsigned(coeff), exp, coeff < 0) {}

    // Comparison Operators
    friend auto operator<(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept
    {
        return lhs.basis_ < rhs.basis_;
    }

    friend auto operator<=(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept
    {
        return lhs.basis_ <= rhs.basis_;
    }

    friend auto operator==(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept
    {
        return lhs.basis_ == rhs.basis_;
    }

    friend auto operator>(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept
    {
        return lhs.basis_ > rhs.basis_;
    }

    friend auto operator>=(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept
    {
        return lhs.basis_ >= rhs.basis_;
    }

    #ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR

    friend auto operator<=>(const hardware_wrapper lhs, const hardware_wrapper rhs) noexcept -> std::partial_ordering;
    {
        if (lhs < rhs)
        {
            return std::partial_ordering::less;
        }
        else if (lhs > rhs)
        {
            return std::partial_ordering::greater;
        }
        else if (lhs == rhs)
        {
            return std::partial_ordering::equivalent;
        }

        return std::partial_ordering::unordered;
    }

    #endif
};

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // Check availability

#endif // BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
