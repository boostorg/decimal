// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This header injects additional functionality into the Boost.Charconv namespace
// If this library is accepted into boost this functionality could be moved their

#ifndef BOOST_DECIMAL_DETAIL_INT128_CHARCONV_HPP
#define BOOST_DECIMAL_DETAIL_INT128_CHARCONV_HPP

#if __has_include(<boost/charconv.hpp>)

// Define for the user automatically,
// otherwise we'll have an ever-increasing number of these required as we go down the dependency chain
#if defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA) && !defined(BOOST_CHARCONV_ENABLE_CUDA)
#  define BOOST_CHARCONV_ENABLE_CUDA
#endif

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/literals.hpp>
#include <boost/charconv.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace charconv {

namespace detail {

template <>
struct is_signed<int128::uint128_t> { static constexpr bool value = false; };

template <>
struct is_signed<int128::int128_t> { static constexpr bool value = true; };

template <>
struct make_unsigned<int128::uint128_t> { using type = int128::uint128_t; };

template <>
struct make_unsigned<int128::int128_t> { using type = int128::uint128_t; };

template <>
struct make_signed<int128::uint128_t> { using type = int128::int128_t; };

template <>
struct make_signed<int128::int128_t> { using type = int128::int128_t; };

#if defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)

template <>
__host__ __device__ constexpr int128::uint128_t get_max_value<int128::uint128_t>()
{
    return std::numeric_limits<int128::uint128_t>::max();
}

template <>
__host__ __device__ constexpr int128::int128_t get_max_value<int128::int128_t>()
{
    return std::numeric_limits<int128::int128_t>::max();
}

#endif // __NVCC__

#if !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR int128::uint128_t int128_pow10[39] =
{
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x1)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0xa)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x64)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x3e8)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2710)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x186a0)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0xf4240)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x989680)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x5f5e100)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x3b9aca00)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2540be400)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x174876e800)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0xe8d4a51000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x9184e72a000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x5af3107a4000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x38d7ea4c68000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2386f26fc10000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x16345785d8a0000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0xde0b6b3a7640000)},
    int128::uint128_t{UINT64_C(0x0), UINT64_C(0x8ac7230489e80000)},
    int128::uint128_t{UINT64_C(0x5), UINT64_C(0x6bc75e2d63100000)},
    int128::uint128_t{UINT64_C(0x36), UINT64_C(0x35c9adc5dea00000)},
    int128::uint128_t{UINT64_C(0x21e), UINT64_C(0x19e0c9bab2400000)},
    int128::uint128_t{UINT64_C(0x152d), UINT64_C(0x2c7e14af6800000)},
    int128::uint128_t{UINT64_C(0xd3c2), UINT64_C(0x1bcecceda1000000)},
    int128::uint128_t{UINT64_C(0x84595), UINT64_C(0x161401484a000000)},
    int128::uint128_t{UINT64_C(0x52b7d2), UINT64_C(0xdcc80cd2e4000000)},
    int128::uint128_t{UINT64_C(0x33b2e3c), UINT64_C(0x9fd0803ce8000000)},
    int128::uint128_t{UINT64_C(0x204fce5e), UINT64_C(0x3e25026110000000)},
    int128::uint128_t{UINT64_C(0x1431e0fae), UINT64_C(0x6d7217caa0000000)},
    int128::uint128_t{UINT64_C(0xc9f2c9cd0), UINT64_C(0x4674edea40000000)},
    int128::uint128_t{UINT64_C(0x7e37be2022), UINT64_C(0xc0914b2680000000)},
    int128::uint128_t{UINT64_C(0x4ee2d6d415b), UINT64_C(0x85acef8100000000)},
    int128::uint128_t{UINT64_C(0x314dc6448d93), UINT64_C(0x38c15b0a00000000)},
    int128::uint128_t{UINT64_C(0x1ed09bead87c0), UINT64_C(0x378d8e6400000000)},
    int128::uint128_t{UINT64_C(0x13426172c74d82), UINT64_C(0x2b878fe800000000)},
    int128::uint128_t{UINT64_C(0xc097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
    int128::uint128_t{UINT64_C(0x785ee10d5da46d9), UINT64_C(0xf436a000000000)},
    int128::uint128_t{UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x98a224000000000)}
};

#endif // __NVCC__

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int num_digits(const int128::uint128_t& x) noexcept
{
    #if defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)

    constexpr int128::uint128_t int128_pow10[39] =
    {
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x1)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0xa)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x64)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x3e8)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2710)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x186a0)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0xf4240)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x989680)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x5f5e100)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x3b9aca00)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2540be400)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x174876e800)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0xe8d4a51000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x9184e72a000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x5af3107a4000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x38d7ea4c68000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x2386f26fc10000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x16345785d8a0000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0xde0b6b3a7640000)},
        int128::uint128_t{UINT64_C(0x0), UINT64_C(0x8ac7230489e80000)},
        int128::uint128_t{UINT64_C(0x5), UINT64_C(0x6bc75e2d63100000)},
        int128::uint128_t{UINT64_C(0x36), UINT64_C(0x35c9adc5dea00000)},
        int128::uint128_t{UINT64_C(0x21e), UINT64_C(0x19e0c9bab2400000)},
        int128::uint128_t{UINT64_C(0x152d), UINT64_C(0x2c7e14af6800000)},
        int128::uint128_t{UINT64_C(0xd3c2), UINT64_C(0x1bcecceda1000000)},
        int128::uint128_t{UINT64_C(0x84595), UINT64_C(0x161401484a000000)},
        int128::uint128_t{UINT64_C(0x52b7d2), UINT64_C(0xdcc80cd2e4000000)},
        int128::uint128_t{UINT64_C(0x33b2e3c), UINT64_C(0x9fd0803ce8000000)},
        int128::uint128_t{UINT64_C(0x204fce5e), UINT64_C(0x3e25026110000000)},
        int128::uint128_t{UINT64_C(0x1431e0fae), UINT64_C(0x6d7217caa0000000)},
        int128::uint128_t{UINT64_C(0xc9f2c9cd0), UINT64_C(0x4674edea40000000)},
        int128::uint128_t{UINT64_C(0x7e37be2022), UINT64_C(0xc0914b2680000000)},
        int128::uint128_t{UINT64_C(0x4ee2d6d415b), UINT64_C(0x85acef8100000000)},
        int128::uint128_t{UINT64_C(0x314dc6448d93), UINT64_C(0x38c15b0a00000000)},
        int128::uint128_t{UINT64_C(0x1ed09bead87c0), UINT64_C(0x378d8e6400000000)},
        int128::uint128_t{UINT64_C(0x13426172c74d82), UINT64_C(0x2b878fe800000000)},
        int128::uint128_t{UINT64_C(0xc097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
        int128::uint128_t{UINT64_C(0x785ee10d5da46d9), UINT64_C(0xf436a000000000)},
        int128::uint128_t{UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x98a224000000000)}
    };

    #endif // __NVCC__

    if (x.high == UINT64_C(0))
    {
        return num_digits(x.low);
    }

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    const auto msb {64 + (63 - int128::detail::countl_zero(x.high))};
    
    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1};

    if (estimated_digits < 39 && x >= int128_pow10[static_cast<std::size_t>(estimated_digits)])
    {
        return estimated_digits + 1;
    }

    // Estimated digits can't be less than 20 digits (65-bits minimum)
    if (x < int128_pow10[estimated_digits - 1])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_CHARCONV_CONSTEXPR to_chars_result to_chars(char* first, char* last, const int128::uint128_t value, const int base = 10) noexcept
{
    #if !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

    if (base == 10)
    {
        return detail::to_chars_128integer_impl<int128::uint128_t, int128::uint128_t>(first, last, value);
    }

    #endif // __NVCC__

    return detail::to_chars_integer_impl<int128::uint128_t, int128::uint128_t>(first, last, value, base);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_CHARCONV_CONSTEXPR to_chars_result to_chars(char* first, char* last, const int128::int128_t value, const int base = 10) noexcept
{
    #if !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

    if (base == 10)
    {
        return detail::to_chars_128integer_impl<int128::int128_t, int128::uint128_t>(first, last, value);
    }

    #endif // __NVCC__

    return detail::to_chars_integer_impl<int128::int128_t, int128::uint128_t>(first, last, value, base);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, int128::uint128_t& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int128::uint128_t, int128::uint128_t>(first, last, value, base);
}

BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(core::string_view sv, int128::uint128_t& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int128::uint128_t, int128::uint128_t>(sv.data(), sv.data() + sv.size(), value, base);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, int128::int128_t& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int128::int128_t, int128::uint128_t>(first, last, value, base);
}

BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(core::string_view sv, int128::int128_t& value, int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int128::int128_t, int128::uint128_t>(sv.data(), sv.data() + sv.size(), value, base);
}

} // namespace charconv
} // namespace boost

#else

#error "This header requires Boost.Charconv to be present"

#endif // __has_include(<boost/charconv.hpp>)

#endif // BOOST_DECIMAL_DETAIL_INT128_CHARCONV_HPP
