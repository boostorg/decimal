// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
#define BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP

#if (__cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)) && \
    (__has_include(<decimal/decimal>) || defined(BOOST_DECIMAL_HAS_BUILTIN_DECIMAL))

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
#include <type_traits>

#endif

// On platforms such as POWER and zSystem the user could have a hardware decimal floating point unit
// This class serves as the wrapper that we then use to specialize support for the rest of the lib.
// We can also wrap GCC's BID type in the event that hardware type doesn't exist,
// which still allows users to utilize that type with the entire standard library that we provide here

namespace boost::decimal::detail {

template <typename BasisType>
class hardware_wrapper
{
private:

    static_assert(std::is_same_v<BasisType, std::decimal::decimal32> ||
                  std::is_same_v<BasisType, std::decimal::decimal64> ||
                  std::is_same_v<BasisType, std::decimal::decimal128>, "Must be one of std::decimal32/64/128");

    static constexpr int value {std::is_same_v<BasisType, std::decimal::decimal32> ? 32 :
                                    std::is_same_v<BasisType, std::decimal::decimal64> ? 64 : 128 };

    using bid_type = std::conditional_t<value == 32, decimal32_t,
                        std::conditional_t<value == 64, decimal64_t, decimal128_t>>;

    #if defined(__s390x__) || (defined(__PPC64__) || defined(__powerpc64__))

    static constexpr bool is_dpd {true};

    #else

    static constexpr bool is_dpd {false};

    #endif // Architectures with DPD

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
        using signed_t1 = detail::make_unsigned_t<T1>;
        signed_t1 new_coeff {};
        if (sign)
        {
            new_coeff = detail::make_signed_value(coeff, sign);
        }
        else
        {
            new_coeff = static_cast<signed_t1>(coeff);
        }

        if constexpr (value == 32)
        {
            basis_ = std::decimal::make_decimal32(coeff, static_cast<int>(exp));
        }
        else if constexpr (value == 64)
        {
            basis_ = std::decimal::make_decimal64(coeff, static_cast<int>(exp));
        }
        else
        {
            basis_ = std::decimal::make_decimal128(coeff, static_cast<int>(exp));
        }
    }

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_SIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<!detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(T1 coeff, T2 exp) noexcept
    {
        if constexpr (value == 32)
        {
            basis_ = std::decimal::make_decimal32(coeff, static_cast<int>(exp));
        }
        else if constexpr (value == 64)
        {
            basis_ = std::decimal::make_decimal64(coeff, static_cast<int>(exp));
        }
        else
        {
            basis_ = std::decimal::make_decimal128(coeff, static_cast<int>(exp));
        }
    }

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

} // namespace boost::decimal::detail

#endif // Check availability

#endif // BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
