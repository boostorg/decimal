// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INTEGER_SEARCH_TREES_HPP
#define BOOST_DECIMAL_DETAIL_INTEGER_SEARCH_TREES_HPP

// https://stackoverflow.com/questions/1489830/efficient-way-to-determine-number-of-digits-in-an-integer?page=1&tab=scoredesc#tab-top
// https://graphics.stanford.edu/~seander/bithacks.html

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/u256.hpp>
#include "int128.hpp"

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstdint>
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {

// Generic solution
template <typename T, std::enable_if_t<std::numeric_limits<T>::digits10 <= std::numeric_limits<std::uint32_t>::digits10, bool> = true>
BOOST_DECIMAL_CUDA_CONSTEXPR auto num_digits(T init_x) noexcept -> int
{
    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    constexpr std::uint32_t powers_of_10_u32[10] =
    {
        UINT32_C(1), UINT32_C(10), UINT32_C(100), UINT32_C(1000), UINT32_C(10000), UINT32_C(100000), UINT32_C(1000000),
        UINT32_C(10000000), UINT32_C(100000000), UINT32_C(1000000000),
    };

    #endif

    const auto x {static_cast<std::uint32_t>(init_x)};

    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto msb {32 - int128::detail::impl::countl_impl(x)};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 10 && x >= powers_of_10_u32[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < powers_of_10_u32[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;

    #else

    // t = bit length * 1233 / 2^12 is floor(log10(2^(bit length))), thus x has t or t + 1 digits.
    // x | 1 has the digits of x, and 1 digit for 0. No branch, as the length of a value is random.
    const auto y {x | 1U};
    const auto t {((32 - int128::detail::impl::countl_impl(y)) * 1233) >> 12};
    return t + 1 - static_cast<int>(y < impl::powers_of_10_u32[t]);

    #endif
}

template <typename T, std::enable_if_t<(std::numeric_limits<T>::digits10 <= std::numeric_limits<std::uint64_t>::digits10) &&
                                       (std::numeric_limits<T>::digits10 > std::numeric_limits<std::uint32_t>::digits10), bool> = true>
BOOST_DECIMAL_CUDA_CONSTEXPR auto num_digits(T init_x) noexcept -> int
{
    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    constexpr std::uint64_t powers_of_10[20] =
    {
        UINT64_C(1), UINT64_C(10), UINT64_C(100), UINT64_C(1000), UINT64_C(10000), UINT64_C(100000), UINT64_C(1000000),
        UINT64_C(10000000), UINT64_C(100000000), UINT64_C(1000000000), UINT64_C(10000000000), UINT64_C(100000000000),
        UINT64_C(1000000000000), UINT64_C(10000000000000), UINT64_C(100000000000000), UINT64_C(1000000000000000),
        UINT64_C(10000000000000000), UINT64_C(100000000000000000), UINT64_C(1000000000000000000),
        UINT64_C(10000000000000000000)
    };

    #endif

    const auto x {static_cast<std::uint64_t>(init_x)};

    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto msb {63 - int128::detail::impl::countl_impl(x)};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 20 && x >= powers_of_10[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < powers_of_10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;

    #else

    // t = bit length * 1233 / 2^12 is floor(log10(2^(bit length))), thus x has t or t + 1 digits.
    // x | 1 has the digits of x, and 1 digit for 0. No branch, as the length of a value is random.
    const auto y {x | 1U};
    const auto t {((64 - int128::detail::impl::countl_impl(y)) * 1233) >> 12};
    return t + 1 - static_cast<int>(y < impl::powers_of_10[t]);

    #endif
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4307) // MSVC 14.1 warns of intergral constant overflow
#endif

BOOST_DECIMAL_CUDA_CONSTEXPR int num_digits(const int128::uint128_t& x) noexcept
{
    if (x.high == UINT64_C(0))
    {
        return num_digits(x.low);
    }

    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    constexpr boost::int128::uint128_t BOOST_DECIMAL_DETAIL_INT128_pow10[39] =
    {
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(1000000000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(10000000000000000000000000000000000000),
        BOOST_DECIMAL_DETAIL_INT128_UINT128_C(100000000000000000000000000000000000000)
    };

    #endif

    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_ENABLE_CUDA)

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto msb {64 + (63 - int128::detail::impl::countl_impl(x.high))};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 39 && x >= BOOST_DECIMAL_DETAIL_INT128_pow10[estimated_digits])
    {
        return estimated_digits + 1;
    }

    // Estimated digits can't be less than 20 (65-bits)
    if (x < BOOST_DECIMAL_DETAIL_INT128_pow10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;

    #else

    // As for 64 bits: x has t or t + 1 digits
    const auto t {((128 - int128::detail::impl::countl_impl(x.high)) * 1233) >> 12};
    return t + 1 - static_cast<int>(x < impl::BOOST_DECIMAL_DETAIL_INT128_pow10[t]);

    #endif
}

BOOST_DECIMAL_CUDA_CONSTEXPR int num_digits(const u256& x) noexcept
{
    if ((x[3] | x[2]) == 0)
    {
        return num_digits(int128::uint128_t{x[1], x[0]});
    }

    // As for 64 bits: x has t or t + 1 digits
    const int b {x[3] != 0U ? 256 - int128::detail::impl::countl_impl(x[3]) : 192 - int128::detail::impl::countl_impl(x[2])};
    const auto t {(b * 1233) >> 12};
    return t + 1 - static_cast<int>(x < pow10_256(static_cast<std::size_t>(t)));
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_DECIMAL_HAS_INT128

constexpr auto num_digits(const builtin_uint128_t& x) noexcept -> int
{
    return num_digits(int128::uint128_t{x});
}

#endif // Has int128

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INTEGER_SEARCH_TREES_HPP
