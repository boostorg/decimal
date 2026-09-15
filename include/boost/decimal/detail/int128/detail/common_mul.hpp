// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_MUL_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_MUL_HPP

#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/detail/fwd.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstdint>

#endif

namespace boost {
namespace int128 {
namespace detail {

template <typename>
struct ctor_high_word
{
    using type = std::uint64_t;
};

template <>
struct ctor_high_word<int128>
{
    using type = std::int64_t;
};

template <typename T>
using ctor_high_word_t = typename ctor_high_word<T>::type;

// High 64 bits of the 64x64 -> 128 product, computed with four 32-bit partial products
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::uint64_t umulh_generic(const std::uint64_t a, const std::uint64_t b) noexcept
{
    const std::uint64_t a_lo {a & UINT32_MAX};
    const std::uint64_t a_hi {a >> 32U};
    const std::uint64_t b_lo {b & UINT32_MAX};
    const std::uint64_t b_hi {b >> 32U};

    const std::uint64_t lo_lo {a_lo * b_lo};
    const std::uint64_t hi_lo {a_hi * b_lo};
    const std::uint64_t lo_hi {a_lo * b_hi};
    const std::uint64_t hi_hi {a_hi * b_hi};

    const std::uint64_t cross {(lo_lo >> 32U) + (hi_lo & UINT32_MAX) + (lo_hi & UINT32_MAX)};

    return hi_hi + (hi_lo >> 32U) + (lo_hi >> 32U) + (cross >> 32U);
}

// Full 64x64 -> 128 product
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::uint64_t umul(const std::uint64_t a, const std::uint64_t b, std::uint64_t& hi) noexcept
{
    #ifndef BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION

    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(a))
    {
        #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

        const detail::builtin_u128 product {static_cast<detail::builtin_u128>(a) * static_cast<detail::builtin_u128>(b)};
        hi = static_cast<std::uint64_t>(product >> 64U);
        return static_cast<std::uint64_t>(product);

        #elif defined(_M_AMD64) && !defined(__GNUC__) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

        return _umul128(a, b, &hi);

        #elif defined(_M_ARM64) && !defined(__GNUC__) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

        hi = __umulh(a, b);
        return a * b;

        #endif
    }

    #endif

    hi = umulh_generic(a, b);
    return a * b;
}

// Low 128 bits of a 128x128 product
template <typename ReturnType, typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr ReturnType low_word_mul(const T& lhs, const T& rhs) noexcept
{
    std::uint64_t result_high {};
    const std::uint64_t result_low {umul(lhs.low, rhs.low, result_high)};

    result_high += lhs.low * static_cast<std::uint64_t>(rhs.high);
    result_high += static_cast<std::uint64_t>(lhs.high) * rhs.low;

    return ReturnType{static_cast<ctor_high_word_t<ReturnType>>(result_high), result_low};
}

// Low 128 bits of a 128x64 product
template <typename ReturnType, typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr ReturnType low_word_mul(const T& lhs, const std::uint64_t rhs) noexcept
{
    std::uint64_t result_high {};
    const std::uint64_t result_low {umul(lhs.low, rhs, result_high)};

    result_high += static_cast<std::uint64_t>(lhs.high) * rhs;

    return ReturnType{static_cast<ctor_high_word_t<ReturnType>>(result_high), result_low};
}

// Low 128 bits of a 128x32 product
template <typename ReturnType, typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr ReturnType low_word_mul(const T& lhs, const std::uint32_t rhs) noexcept
{
    return low_word_mul<ReturnType>(lhs, static_cast<std::uint64_t>(rhs));
}

} // namespace detail
} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_MUL_HPP
