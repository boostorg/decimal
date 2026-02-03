// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/impl/pow_impl.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <type_traits>
#include <limits>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto frexp_impl(const T v, int* expon) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    // This implementation of frexp follows closely that of eval_frexp
    // in Boost.Multiprecision's cpp_dec_float template class.
    constexpr T zero { 0, 0 };

    auto result_frexp = zero;

    const auto v_fp {fpclassify(v)};

    if (v_fp != FP_NORMAL)
    {
        if (expon != nullptr) { *expon = 0; }

        #ifndef BOOST_DECIMAL_FAST_MATH
        if (v_fp == FP_NAN)
        {
            result_frexp = std::numeric_limits<T>::quiet_NaN();
        }
        else if (v_fp == FP_INFINITE)
        {
            result_frexp = std::numeric_limits<T>::infinity();
        }
        #endif
    }
    else
    {
        result_frexp = v;

        const auto b_neg = signbit(v);

        if(b_neg) { result_frexp = -result_frexp; }

        int t { };
        static_cast<void>(frexp10(result_frexp, &t));

        t = (t * 1000) / 301;

        result_frexp *= detail::pow_2_impl<T>(-t);

        // TODO(ckormanyos): Handle underflow/overflow if (or when) needed.

        constexpr T two_pow_16 { UINT32_C(65536) };

        while (result_frexp >= two_pow_16)
        {
            result_frexp = result_frexp / two_pow_16;
            t += 16;
        }

        constexpr T two_pow_8 { UINT16_C(256) };

        while (result_frexp >= two_pow_8)
        {
            result_frexp = result_frexp / two_pow_8;
            t += 8;
        }

        constexpr T local_one { 1, 0 };
        constexpr T local_two { 2, 0 };

        while (result_frexp >= local_one)
        {
            result_frexp /= local_two;

            ++t;
        }

        constexpr T local_half { 5, -1 };

        while (result_frexp < local_half)
        {
            result_frexp *= local_two;

            --t;
        }

        if (expon != nullptr) { *expon = t; }

        if(b_neg) { result_frexp = -result_frexp; }
    }

    return result_frexp;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto frexp(const T v, int* expon) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::frexp_impl(static_cast<evaluation_type>(v), expon));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP
