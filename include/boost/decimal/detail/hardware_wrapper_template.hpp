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
#include <type_traits>

#endif

// On platforms such as POWER and zSystem the user could have a hardware decimal floating point unit
// This class serves as the wrapper that we then use to specialize support for the rest of the lib.
// We can also wrap GCC's BID type in the event that hardware type doesn't exist,
// which still allows users to utilize that type with the entire standard library that we provide here

namespace boost {
namespace decimal {
namespace detail {

#if defined(__s390x__) || (defined(__PPC64__) || defined(__powerpc64__))

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE bool _is_dpd {true};

#else

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE bool _is_dpd {false};

#endif // Architectures with DPD

template <typename T>
T make_builtin_decimal(long long coeff, int exp);

template <typename T>
T make_builtin_decimal(unsigned long long coeff, int exp);

template <>
inline std::decimal::decimal32 make_builtin_decimal<std::decimal::decimal32>(const long long coeff, const int exp)
{
    return std::decimal::make_decimal32(coeff, exp);
}

template <>
inline std::decimal::decimal32 make_builtin_decimal<std::decimal::decimal32>(const unsigned long long coeff, const int exp)
{
    return std::decimal::make_decimal32(coeff, exp);
}

template <>
inline std::decimal::decimal64 make_builtin_decimal<std::decimal::decimal64>(const long long coeff, const int exp)
{
    return std::decimal::make_decimal64(coeff, exp);
}

template <>
inline std::decimal::decimal64 make_builtin_decimal<std::decimal::decimal64>(const unsigned long long coeff, const int exp)
{
    return std::decimal::make_decimal64(coeff, exp);
}

template <>
inline std::decimal::decimal128 make_builtin_decimal<std::decimal::decimal128>(const long long coeff, const int exp)
{
    return std::decimal::make_decimal128(coeff, exp);
}

template <>
inline std::decimal::decimal128 make_builtin_decimal<std::decimal::decimal128>(const unsigned long long coeff, const int exp)
{
    return std::decimal::make_decimal128(coeff, exp);
}

template <typename BasisType>
class hardware_wrapper
{
private:

    static_assert(std::is_same<BasisType, std::decimal::decimal32>::value ||
                  std::is_same<BasisType, std::decimal::decimal64>::value ||
                  std::is_same<BasisType, std::decimal::decimal128>::value, "Must be one of: std::decimal::decimal32/64/128");

    static constexpr int value_ {std::is_same<BasisType, std::decimal::decimal32>::value ? 32 :
                                    std::is_same<BasisType, std::decimal::decimal64>::value ? 64 : 128 };

    using bid_type = std::conditional_t<value_ == 32, decimal32_t,
                        std::conditional_t<value_ == 64, decimal64_t, decimal128_t>>;

public:

    using significand_type = typename bid_type::significand_type;
    using exponent_type = typename bid_type::exponent_type;
    using biased_exponent_type = typename bid_type::biased_exponent_type;

private:

    BasisType basis_ {};

public:

    explicit hardware_wrapper(const BasisType value) : basis_(value) {}

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_UNSIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(T1 coeff, T2 exp = 0, bool sign = false)
    {
        if (sign)
        {
            const auto new_coeff {detail::make_signed_value(coeff, sign)};
            basis_ = make_builtin_decimal<BasisType>(static_cast<long long>(new_coeff), static_cast<int>(exp));
        }
        else
        {
            basis_ = make_builtin_decimal<BasisType>(static_cast<unsigned long long>(coeff), static_cast<int>(exp));
        }
    }

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_SIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<!detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(T1 coeff, T2 exp = 0)
    {
        basis_ = make_builtin_decimal<BasisType>(coeff, static_cast<int>(exp));
    }

    // Comparison Operators
    friend auto operator<(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ < rhs.basis_;
    }

    friend auto operator<=(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ <= rhs.basis_;
    }

    friend auto operator==(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ == rhs.basis_;
    }

    friend auto operator!=(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ != rhs.basis_;
    }

    friend auto operator>(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ > rhs.basis_;
    }

    friend auto operator>=(const hardware_wrapper lhs, const hardware_wrapper rhs)
    {
        return lhs.basis_ >= rhs.basis_;
    }

    template <typename T1, typename T2>
    friend bool operator<(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    template <typename T1, typename T2>
    friend bool operator<=(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    template <typename T1, typename T2>
    friend bool operator==(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    template <typename T1, typename T2>
    friend bool operator!=(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    template <typename T1, typename T2>
    friend bool operator>(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    template <typename T1, typename T2>
    friend bool operator>=(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs);

    operator long long() const
    {
        return std::decimal::decimal_to_long_long(basis_);
    }

    explicit operator float() const
    {
        return std::decimal::decimal_to_float(basis_);
    }

    explicit operator double() const
    {
        return std::decimal::decimal_to_double(basis_);
    }

    #ifndef BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE
    explicit operator long double() const
    {
        return std::decimal::decimal_to_long_double(basis_);
    }
    #endif // BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE

    // Unary Arithmetic Operators
    friend hardware_wrapper operator+(const hardware_wrapper rhs)
    {
        return hardware_wrapper{+rhs.basis_};
    }

    friend hardware_wrapper operator-(const hardware_wrapper rhs)
    {
        return hardware_wrapper{-rhs.basis_};
    }

    // Binary Arithmetic Operators
    // Addition
    friend hardware_wrapper operator+(const hardware_wrapper lhs, const int rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const int lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    friend hardware_wrapper operator+(const hardware_wrapper lhs, const unsigned int rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const unsigned int lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    friend hardware_wrapper operator+(const hardware_wrapper lhs, const long rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const long lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    friend hardware_wrapper operator+(const hardware_wrapper lhs, const unsigned long rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const unsigned long lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    friend hardware_wrapper operator+(const hardware_wrapper lhs, const long long rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const long long lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    friend hardware_wrapper operator+(const hardware_wrapper lhs, const unsigned long long rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    friend hardware_wrapper operator+(const unsigned long long lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    template <typename T1, typename T2>
    friend auto operator+(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs)
        -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;
};

template <typename T1, typename T2>
bool operator<(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ < rhs.basis_;
}

template <typename T1, typename T2>
bool operator<=(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ <= rhs.basis_;
}

template <typename T1, typename T2>
bool operator==(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ == rhs.basis_;
}

template <typename T1, typename T2>
bool operator!=(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ != rhs.basis_;
}

template <typename T1, typename T2>
bool operator>(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ > rhs.basis_;
}

template <typename T1, typename T2>
bool operator>=(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
{
    return lhs.basis_ >= rhs.basis_;
}

#ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR

template <typename T1, typename T2>
auto operator<=>(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs) -> std::partial_ordering;
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

template <typename T1, typename T2>
auto operator+(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
    -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>
{
    using return_type = std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;
    return return_type{lhs.basis_ + rhs.basis_};
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // Check availability

#endif // BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
