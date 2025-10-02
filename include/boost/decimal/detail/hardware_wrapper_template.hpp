// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
#define BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP

#include <boost/decimal/detail/config.hpp>

#ifdef BOOST_DECIMAL_HAS_BUILTIN_DECIMAL

#include <boost/decimal/fwd.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE

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

namespace impl {

template <typename>
struct supported_integer
{
    static constexpr bool value = false;
};

template <>
struct supported_integer<int>
{
    static constexpr bool value = true;
};

template <>
struct supported_integer<unsigned int>
{
    static constexpr bool value = true;
};

template <>
struct supported_integer<long>
{
    static constexpr bool value = true;
};

template <>
struct supported_integer<unsigned long>
{
    static constexpr bool value = true;
};

template <>
struct supported_integer<long long>
{
    static constexpr bool value = true;
};

template <>
struct supported_integer<unsigned long long>
{
    static constexpr bool value = true;
};

} // namespace impl

template <typename T>
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE bool is_supported_integer_v = impl::supported_integer<T>::value;

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

    template <typename OtherBasis>
    friend class hardware_wrapper;

public:

    hardware_wrapper() = default;

    explicit hardware_wrapper(const BasisType value) : basis_{value} {}

    template <typename OtherBasis>
    explicit hardware_wrapper(const hardware_wrapper<OtherBasis> other) : basis_{static_cast<BasisType>(other.basis_)} {}

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_UNSIGNED_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
    #else
    template <typename T1, typename T2, std::enable_if_t<detail::is_unsigned_v<T1> && detail::is_integral_v<T2>, bool> = true>
    #endif
    hardware_wrapper(const T1 coeff, const T2 exp, const bool sign = false)
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
    hardware_wrapper(const T1 coeff, const T2 exp)
    {
        basis_ = make_builtin_decimal<BasisType>(coeff, static_cast<int>(exp));
    }

    #ifdef BOOST_DECIMAL_HAS_CONCEPTS
    template <BOOST_DECIMAL_INTEGRAL T1>
    #else
    template <typename T1, std::enable_if_t<detail::is_integral_v<T1>, bool> = true>
    #endif
    hardware_wrapper(const T1 coeff)
    {
        basis_ = make_builtin_decimal<BasisType>(coeff, static_cast<int>(0));
    }

    explicit hardware_wrapper(const float value)
    {
        basis_ = static_cast<BasisType>(value);
    }

    explicit hardware_wrapper(const double value)
    {
        basis_ = static_cast<BasisType>(value);
    }

    #ifndef BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE
    explicit hardware_wrapper(const long double value)
    {
        basis_ = static_cast<BasisType>(value);
    }
    #endif

    // Comparison Operators
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

    // Conversion operators
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

    template <typename OtherBasis>
    explicit operator hardware_wrapper<OtherBasis>() const
    {
        hardware_wrapper<OtherBasis> result;
        result.basis_ = static_cast<OtherBasis>(basis_);
        return result;
    }

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
    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator+(const hardware_wrapper lhs, const IntegerType rhs)
    {
        return hardware_wrapper{lhs.basis_ + rhs};
    }

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator+(const IntegerType lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs + rhs.basis_};
    }

    template <typename T1, typename T2>
    friend auto operator+(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs)
        -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;

    // Subtraction
    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator-(const hardware_wrapper lhs, const IntegerType rhs)
    {
        return hardware_wrapper{lhs.basis_ - rhs};
    }

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator-(const IntegerType lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs - rhs.basis_};
    }

    template <typename T1, typename T2>
    friend auto operator-(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs)
        -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;

    // Multiplication
    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator*(const hardware_wrapper lhs, const IntegerType rhs)
    {
        return hardware_wrapper{lhs.basis_ * rhs};
    }

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator*(const IntegerType lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs * rhs.basis_};
    }

    template <typename T1, typename T2>
    friend auto operator*(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs)
        -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;

    // Division
    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator/(const hardware_wrapper lhs, const IntegerType rhs)
    {
        return hardware_wrapper{lhs.basis_ / rhs};
    }

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    friend hardware_wrapper operator/(const IntegerType lhs, const hardware_wrapper rhs)
    {
        return hardware_wrapper{lhs / rhs.basis_};
    }

    template <typename T1, typename T2>
    friend auto operator/(hardware_wrapper<T1> lhs, hardware_wrapper<T2> rhs)
        -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;

    // Compound Operators
    hardware_wrapper& operator++();
    hardware_wrapper operator++(int);

    hardware_wrapper& operator--();
    hardware_wrapper operator--(int);

    template <typename OtherBasis>
    hardware_wrapper& operator+=(hardware_wrapper<OtherBasis> rhs);

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    hardware_wrapper& operator+=(IntegerType rhs);

    template <typename OtherBasis>
    hardware_wrapper& operator-=(hardware_wrapper<OtherBasis> rhs);

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    hardware_wrapper& operator-=(IntegerType rhs);

    template <typename OtherBasis>
    hardware_wrapper& operator*=(hardware_wrapper<OtherBasis> rhs);

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    hardware_wrapper& operator*=(IntegerType rhs);

    template <typename OtherBasis>
    hardware_wrapper& operator/=(hardware_wrapper<OtherBasis> rhs);

    template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool> = true>
    hardware_wrapper& operator/=(IntegerType rhs);
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

template <typename T1, typename T2>
auto operator-(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
    -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>
{
    using return_type = std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;
    return return_type{lhs.basis_ - rhs.basis_};
}

template <typename T1, typename T2>
auto operator*(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
    -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>
{
    using return_type = std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;
    return return_type{lhs.basis_ * rhs.basis_};
}

template <typename T1, typename T2>
auto operator/(const hardware_wrapper<T1> lhs, const hardware_wrapper<T2> rhs)
    -> std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>
{
    using return_type = std::conditional_t<(sizeof(T1) > sizeof(T2)), hardware_wrapper<T1>, hardware_wrapper<T2>>;
    return return_type{lhs.basis_ / rhs.basis_};
}

template <typename BasisType>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator++()
{
    ++basis_;
    return *this;
}

template <typename BasisType>
hardware_wrapper<BasisType> hardware_wrapper<BasisType>::operator++(int)
{
    const auto tmp {*this};
    ++basis_;
    return tmp;
}

template <typename BasisType>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator--()
{
    --basis_;
    return *this;
}

template <typename BasisType>
hardware_wrapper<BasisType> hardware_wrapper<BasisType>::operator--(int)
{
    const auto tmp {*this};
    --basis_;
    return tmp;
}

template <typename BasisType>
template <typename OtherBasis>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator+=(hardware_wrapper<OtherBasis> rhs)
{
    basis_ = static_cast<BasisType>(basis_ + rhs.basis_);
    return *this;
}

template <typename BasisType>
template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool>>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator+=(IntegerType rhs)
{
    basis_ += rhs;
    return *this;
}

template <typename BasisType>
template <typename OtherBasis>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator-=(hardware_wrapper<OtherBasis> rhs)
{
    basis_ = static_cast<BasisType>(basis_ - rhs.basis_);
    return *this;
}

template <typename BasisType>
template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool>>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator-=(IntegerType rhs)
{
    basis_ -= rhs;
    return *this;
}

template <typename BasisType>
template <typename OtherBasis>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator*=(hardware_wrapper<OtherBasis> rhs)
{
    basis_ = static_cast<BasisType>(basis_ * rhs.basis_);
    return *this;
}

template <typename BasisType>
template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool>>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator*=(IntegerType rhs)
{
    basis_ *= rhs;
    return *this;
}

template <typename BasisType>
template <typename OtherBasis>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator/=(hardware_wrapper<OtherBasis> rhs)
{
    basis_ = static_cast<BasisType>(basis_ / rhs.basis_);
    return *this;
}

template <typename BasisType>
template <typename IntegerType, std::enable_if_t<is_supported_integer_v<IntegerType>, bool>>
hardware_wrapper<BasisType>& hardware_wrapper<BasisType>::operator/=(IntegerType rhs)
{
    basis_ /= rhs;
    return *this;
}

template <typename BasisType>
bool signbit     BOOST_DECIMAL_PREVENT_MACRO_SUBSTITUTION (const hardware_wrapper<BasisType> rhs)
{
    return rhs < 0;
}

template <typename BasisType>
bool isinf       BOOST_DECIMAL_PREVENT_MACRO_SUBSTITUTION (const hardware_wrapper<BasisType> rhs)
{
    return rhs == std::numeric_limits<hardware_wrapper<BasisType>>::infinity();
}

template <typename BasisType>
bool isnan       BOOST_DECIMAL_PREVENT_MACRO_SUBSTITUTION (const hardware_wrapper<BasisType> rhs)
{
    return rhs != rhs;
}

template <typename BasisType>
bool isnormal    BOOST_DECIMAL_PREVENT_MACRO_SUBSTITUTION (const hardware_wrapper<BasisType> rhs)
{
    // TODO(mborland): Once we can decode values we can check for sub-normals
    return rhs != 0 && isfinite(rhs);
}

template <typename BasisType>
bool isfinite    BOOST_DECIMAL_PREVENT_MACRO_SUBSTITUTION (const hardware_wrapper<BasisType> rhs)
{
    return !isinf(rhs) && !isnan(rhs);
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // Check availability

#endif // BOOST_DECIMAL_DETAIL_HARDWARE_WRAPPER_TEMPLATE_HPP
