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
constexpr auto num_digits(T init_x) noexcept -> int
{
    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto x {static_cast<std::uint32_t>(init_x)};

    const auto msb {32 - int128::detail::impl::countl_impl(x)};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 10 && x >= impl::powers_of_10_u32[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < impl::powers_of_10_u32[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

template <typename T, std::enable_if_t<(std::numeric_limits<T>::digits10 <= std::numeric_limits<std::uint64_t>::digits10) &&
                                       (std::numeric_limits<T>::digits10 > std::numeric_limits<std::uint32_t>::digits10), bool> = true>
constexpr auto num_digits(T init_x) noexcept -> int
{
    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto x {static_cast<std::uint64_t>(init_x)};

    const auto msb {63 - int128::detail::impl::countl_impl(x)};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 20 && x >= impl::powers_of_10[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < impl::powers_of_10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4307) // MSVC 14.1 warns of intergral constant overflow
#endif

constexpr int num_digits(const boost::int128::uint128_t& x) noexcept
{
    if (x.high == UINT64_C(0))
    {
        return num_digits(x.low);
    }

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto msb {64 + (63 - int128::detail::impl::countl_impl(x.high))};

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 39 && x >= impl::BOOST_DECIMAL_DETAIL_INT128_pow10[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < impl::BOOST_DECIMAL_DETAIL_INT128_pow10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

constexpr int num_digits(const u256& x) noexcept
{
    if ((x[3] | x[2]) == 0)
    {
        return num_digits(int128::uint128_t{x[1], x[0]});
    }

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    int msb {};
    if (x[3] != 0U)
    {
        msb = 192 + (63 - int128::detail::impl::countl_impl(x[3]));
    }
    else
    {
        msb = 128 + (63 - int128::detail::impl::countl_impl(x[2]));
    }

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1}; // 1000/3322 ~= 1/log2(10)

    if (estimated_digits < 78 && x >= impl::u256_pow_10[estimated_digits])
    {
        return estimated_digits + 1;
    }

    if (estimated_digits > 1 && x < impl::u256_pow_10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_DECIMAL_HAS_INT128

constexpr auto num_digits(const builtin_uint128_t& x) noexcept -> int
{
    if (static_cast<std::uint64_t>(x >> 64) == UINT64_C(0))
    {
        return num_digits(static_cast<std::uint64_t>(x));
    }

    // We start left at 19 because we already eliminated the high word being 0
    std::uint32_t left = 19U;
    std::uint32_t right = 38U;

    while (left < right)
    {
        std::uint32_t mid = (left + right + 1U) / 2U;

        if (x >= impl::builtin_128_pow10[mid])
        {
            left = mid;
        }
        else
        {
            right = mid - 1;
        }
    }

    return static_cast<int>(left + 1);
}

#endif // Has int128

// Specializations with pruned branches for constructors
// Since we already have partial information we can greatly speed things up in this case
template <typename T>
constexpr auto d32_constructor_num_digits(T) noexcept -> std::enable_if_t<std::numeric_limits<T>::digits10 + 1 < 7, int>
{
    // Does not matter since it is guaranteed to fit
    return 0;
}

template <typename T>
constexpr auto d32_constructor_num_digits(T x) noexcept -> std::enable_if_t<(std::numeric_limits<T>::digits10 + 1 <= 10) &&
                                                                            (std::numeric_limits<T>::digits10 + 1 > 7), int>
{
    BOOST_DECIMAL_ASSERT(x >= 10000000);

    if (x >= 100000000)
    {
        if (x >= 1000000000)
        {
            return 10;
        }
        return 9;
    }
    return 8;
}

template <typename T>
constexpr auto d32_constructor_num_digits(T x) noexcept -> std::enable_if_t<(std::numeric_limits<T>::digits10 + 1 > 10) &&
                                                                            (std::numeric_limits<T>::digits10 + 1 <= 20), int>
{
    // We already know that x >= 10000000 (7 digits)
    BOOST_DECIMAL_ASSERT(x >= 10000000);

    if (x >= UINT64_C(10000000000))
    {
        if (x >= UINT64_C(100000000000000))
        {
            if (x >= UINT64_C(10000000000000000))
            {
                if (x >= UINT64_C(100000000000000000))
                {
                    if (x >= UINT64_C(1000000000000000000))
                    {
                        if (x >= UINT64_C(10000000000000000000))
                        {
                            return 20;
                        }
                        return 19;
                    }
                    return 18;
                }
                return 17;
            }
            else if (x >= UINT64_C(1000000000000000))
            {
                return 16;
            }
            return 15;
        }
        if (x >= UINT64_C(1000000000000))
        {
            if (x >= UINT64_C(10000000000000))
            {
                return 14;
            }
            return 13;
        }
        if (x >= UINT64_C(100000000000))
        {
            return 12;
        }
        return 11;
    }
    else // 10000000 <= x < 10000000000
    {
        if (x >= UINT64_C(100000000))
        {
            if (x >= UINT64_C(1000000000))
            {
                return 10;
            }
            return 9;
        }
        else // 10000000 <= x < 100000000
        {
            return 8;
        }
    }
}

template <typename T>
constexpr auto d32_constructor_num_digits(T x) noexcept -> std::enable_if_t<(std::numeric_limits<T>::digits10 + 1 > 20), int>
{
    // Anything bigger than uint64_t has no benefit so fall back to that
    return num_digits(x);
}

#ifdef BOOST_DECIMAL_HAS_INT128
constexpr auto d32_constructor_num_digits(builtin_uint128_t x) noexcept -> int
{
    return num_digits(x);
}
#endif

template <typename T>
constexpr auto d64_constructor_num_digits(T) noexcept -> std::enable_if_t<(std::numeric_limits<T>::digits10 + 1 <= 16), int>
{
    return 0;
}

template <typename T>
constexpr auto d64_constructor_num_digits(T x) noexcept -> std::enable_if_t<(std::numeric_limits<T>::digits10 + 1 > 16) &&
                                                                            (std::numeric_limits<T>::digits10 <= 20), int>
{
    // Pre-condition: x >= 10^16
    BOOST_DECIMAL_ASSERT(x >= UINT64_C(10000000000000000));

    if (x >= UINT64_C(100000000000000000))
    {
        if (x >= UINT64_C(1000000000000000000))
        {
            if (x >= UINT64_C(10000000000000000000))
            {
                return 20;
            }
            return 19;
        }
        return 18;
    }
    return 17;
}

template <typename T>
constexpr auto d64_constructor_num_digits(T x) noexcept -> std::enable_if_t<std::numeric_limits<T>::digits10 >= 20, int>
{
    return num_digits(x);
}

#ifdef BOOST_DECIMAL_HAS_INT128
constexpr auto d64_constructor_num_digits(builtin_uint128_t x) noexcept -> int
{
    return num_digits(x);
}
#endif

template <typename T>
constexpr auto d128_constructor_num_digits(T) noexcept -> std::enable_if_t<std::numeric_limits<T>::digits10 + 1 <= 34, int>
{
    return 0;
}

#ifdef BOOST_DECIMAL_HAS_INT128
constexpr auto d128_constructor_num_digits(builtin_uint128_t x) noexcept -> int
{
    // Pre-condition: we know x has at least 34 digits
    BOOST_DECIMAL_ASSERT(x >= detail::pow10(static_cast<builtin_uint128_t>(33)));

    constexpr auto digits35 {detail::pow10(static_cast<builtin_uint128_t>(34))};
    constexpr auto digits36 {detail::pow10(static_cast<builtin_uint128_t>(35))};
    constexpr auto digits37 {detail::pow10(static_cast<builtin_uint128_t>(36))};
    constexpr auto digits38 {detail::pow10(static_cast<builtin_uint128_t>(37))};
    constexpr auto digits39 {detail::pow10(static_cast<builtin_uint128_t>(38))};

    if (x >= digits38)
    {
        if (x >= digits39)
        {
            return 39;
        }
        return 38;
    }
    if (x >= digits36)
    {
        if (x >= digits37)
        {
            return 37;
        }
        return 36;
    }
    if (x >= digits35)
    {
        return 35;
    }

    return 34;  // Since we know x has at least 34 digits
}
#endif

constexpr auto d128_constructor_num_digits(const boost::int128::uint128_t x) noexcept -> int
{
    // Pre-condition: we know x has at least 34 digits
    BOOST_DECIMAL_ASSERT(x >= detail::pow10(static_cast<boost::int128::uint128_t>(33)));

    // Since we know that x has at least 34 digits we can get away with just comparing the high bits,
    // which reduces these to uint64_t comps instead of synthesized 128-bit

    constexpr auto digits35 {detail::pow10(static_cast<boost::int128::uint128_t>(34)).high};
    constexpr auto digits36 {detail::pow10(static_cast<boost::int128::uint128_t>(35)).high};
    constexpr auto digits37 {detail::pow10(static_cast<boost::int128::uint128_t>(36)).high};
    constexpr auto digits38 {detail::pow10(static_cast<boost::int128::uint128_t>(37)).high};
    constexpr auto digits39 {detail::pow10(static_cast<boost::int128::uint128_t>(38)).high};

    const auto x_high {x.high};

    if (x_high >= digits38)
    {
        if (x_high >= digits39)
        {
            return 39;
        }
        return 38;
    }
    if (x_high >= digits36)
    {
        if (x_high >= digits37)
        {
            return 37;
        }
        return 36;
    }
    if (x_high >= digits35)
    {
        return 35;
    }

    return 34;  // Since we know x has at least 34 digits
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INTEGER_SEARCH_TREES_HPP
