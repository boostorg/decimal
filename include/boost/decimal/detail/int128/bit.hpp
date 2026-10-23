// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_BIT_HPP
#define BOOST_DECIMAL_DETAIL_INT128_BIT_HPP

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/detail/clz.hpp>
#include <boost/decimal/detail/int128/detail/ctz.hpp>

namespace boost {
namespace int128 {

namespace impl {

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countl_zero_impl(const uint128 x) noexcept
{
    return x.high == 0 ? 64 + detail::countl_zero(x.low) : detail::countl_zero(x.high);
}

} // namespace impl

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countl_zero(const uint128 x) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)) && BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_clzg) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::countl_zero_impl(x);
    }

    // The second argument is the result for x == 0, which is undefined without it
    return __builtin_clzg(static_cast<detail::builtin_u128>(x), 128);

    #else

    return impl::countl_zero_impl(x);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countl_one(const uint128 x) noexcept
{
    return countl_zero(~x);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int bit_width(const uint128 x) noexcept
{
    return x ? 128 - countl_zero(x) : 0;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 bit_ceil(const uint128 x) noexcept
{
    // __builtin_stdc_bit_ceil not available, but this is equivalent
    return x <= 1U ? static_cast<uint128>(1) : static_cast<uint128>(2) << (127 - countl_zero(x - 1));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 bit_floor(const uint128 x) noexcept
{
    // __builtin_stdc_bit_floor not available, but this is equivalent
    return x == 0U ? static_cast<uint128>(0) : static_cast<uint128>(1) << (127 - countl_zero(x));
}

namespace impl {

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countr_zero_impl(const uint128 x) noexcept
{
    return x.low == 0 ? 64 + detail::countr_zero(x.high) : detail::countr_zero(x.low);
}

} // namespace impl

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countr_zero(const uint128 x) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)) && BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_ctzg) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::countr_zero_impl(x);
    }

    // The second argument is the result for x == 0, which is undefined without it
    return __builtin_ctzg(static_cast<detail::builtin_u128>(x), 128);

    #else

    return impl::countr_zero_impl(x);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int countr_one(const uint128 x) noexcept
{
    return countr_zero(~x);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 rotl(const uint128 x, const int s) noexcept
{
    // __builtin_stdc_rotate_left not available
    constexpr auto mask {127U};
    return x << (static_cast<unsigned>(s) & mask) | x >> (static_cast<unsigned>(-s) & mask);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 rotr(const uint128 x, const int s) noexcept
{
    // __builtin_stdc_rotate_right not available
    constexpr auto mask {127U};
    return x >> (static_cast<unsigned>(s) & mask) | x << (static_cast<unsigned>(-s) & mask);
}

namespace impl {

BOOST_int128EST_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int popcount_impl(std::uint64_t x) noexcept
{
    x = x - ((x >> 1U) & UINT64_C(0x5555555555555555));
    x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2U) & UINT64_C(0x3333333333333333));
    x = (x + (x >> 4U)) & UINT64_C(0x0F0F0F0F0F0F0F0F);

    return static_cast<int>((x * UINT64_C(0x0101010101010101)) >> 56U);
}

// The exact-match overload above is selected for the 64-bit halves
BOOST_int128EST_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int popcount_impl(const uint128 x) noexcept
{
    return popcount_impl(x.high) + popcount_impl(x.low);
}

} // namespace impl

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int popcount(const uint128 x) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)) && BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_popcountg) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::popcount_impl(x);
    }

    return __builtin_popcountg(static_cast<detail::builtin_u128>(x));

    #elif BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_popcountll) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

    return __builtin_popcountll(x.high) + __builtin_popcountll(x.low);

    #elif defined(_M_AMD64) && !defined(__GNUC__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::popcount_impl(x); // LCOV_EXCL_LINE
    }
    else
    {
        #ifdef __AVX__

        return static_cast<int>(_mm_popcnt_u64(x.high) +  _mm_popcnt_u64(x.low));

        #else

        return static_cast<int>(__popcnt64(x.high) + __popcnt64(x.low));

        #endif
    }

    #elif defined(_M_IX86) && !defined(__GNUC__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::popcount_impl(x); // LCOV_EXCL_LINE
    }
    else
    {
        #ifdef __AVX__

        return static_cast<int>(
               _mm_popcnt_u32(static_cast<unsigned>(x.high)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.high >> 32U)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.low)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.low >> 32U)));

        #else

        return static_cast<int>(
               __popcnt(static_cast<unsigned>(x.high)) +
               __popcnt(static_cast<unsigned>(x.high >> 32U)) +
               __popcnt(static_cast<unsigned>(x.low)) +
               __popcnt(static_cast<unsigned>(x.low >> 32U)));

        #endif
    }

    #else

    return impl::popcount_impl(x);

    #endif
}

namespace impl {

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::uint64_t byteswap_impl(const std::uint64_t x) noexcept
{
    const auto step32 {x << 32U | x >> 32U};
    const auto step16 {(step32 & UINT64_C(0x0000FFFF0000FFFF)) << 16U | (step32 & UINT64_C(0xFFFF0000FFFF0000)) >> 16U};
    return (step16 & UINT64_C(0x00FF00FF00FF00FF)) << 8U | (step16 & UINT64_C(0xFF00FF00FF00FF00)) >> 8U;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 byteswap_impl(const uint128 x) noexcept
{
    return {byteswap_impl(x.low), byteswap_impl(x.high)};
}

} // namespace impl

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 byteswap(const uint128 x) noexcept
{
    // The whole-width builtins are deliberately ranked below the paired 64-bit form.
    // Measured today (7/29/2026) they are a regression: the 128-bit value blocks the loop vectorization
    // the paired __builtin_bswap64 receives, costing up to 1.5x on arm64, and on x86-64 both
    // forms emit identical code. Revisit if the codegen improves.
    #if BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_bswap64) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

    return {__builtin_bswap64(x.low), __builtin_bswap64(x.high)};

    // __builtin_bswapg is clang-only (LLVM 22.1) and __builtin_bswap128 is GCC-only (GCC 11),
    // so at most one of the two whole-width branches is ever live
    #elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)) && BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_bswapg) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::byteswap_impl(x);
    }

    return static_cast<uint128>(__builtin_bswapg(static_cast<detail::builtin_u128>(x)));

    #elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA)) && BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN(__builtin_bswap128) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::byteswap_impl(x);
    }

    return static_cast<uint128>(__builtin_bswap128(static_cast<detail::builtin_u128>(x)));

    #elif defined(_MSC_VER) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        return impl::byteswap_impl(x); // LCOV_EXCL_LINE
    }
    else
    {
        return {_byteswap_uint64(x.low), _byteswap_uint64(x.high)};
    }

    #else

    return impl::byteswap_impl(x);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool has_single_bit(const uint128 x) noexcept
{
    return popcount(x) == 1;
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_BIT_HPP
