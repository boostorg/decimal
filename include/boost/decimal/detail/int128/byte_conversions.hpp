// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_BYTE_CONVERSIONS_HPP
#define BOOST_DECIMAL_DETAIL_INT128_BYTE_CONVERSIONS_HPP

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/bit.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#endif

namespace boost {
namespace int128 {

//=====================================
// Whole value byte order conversions
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 to_be(const uint128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_BIG_BYTE

    return value;

    #else

    return byteswap(value);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 to_be(const int128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_BIG_BYTE

    return value;

    #else

    // Reversing the two's complement bit pattern is the same operation for both signs
    return static_cast<int128>(byteswap(static_cast<uint128>(value)));

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 from_be(const uint128 value) noexcept
{
    // Self-inverse
    return to_be(value);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 from_be(const int128 value) noexcept
{
    // Self-inverse
    return to_be(value);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 to_le(const uint128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return value;

    #else

    return byteswap(value);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 to_le(const int128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return value;

    #else

    // Reversing the two's complement bit pattern is the same operation for both signs
    return static_cast<int128>(byteswap(static_cast<uint128>(value)));

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 from_le(const uint128 value) noexcept
{
    // Self-inverse
    return to_le(value);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 from_le(const int128 value) noexcept
{
    // Self-inverse
    return to_le(value);
}

namespace detail {

// The byte-like element types that the byte array functions operate on.
// std::byte only exists from C++17, and the library is usable from C++14.
template <typename T>
struct byte_like
{
    static constexpr bool value = std::is_same<T, char>::value ||
                                  std::is_same<T, signed char>::value ||
                                  std::is_same<T, unsigned char>::value
                                  #if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603L
                                  || std::is_same<T, std::byte>::value
                                  #endif
                                  ;
};

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_byte_like_v = byte_like<T>::value;

// Reads the byte sitting at bit offset shift of a 64-bit word
template <typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr ByteType extract_byte(const std::uint64_t word, const unsigned shift) noexcept
{
    return static_cast<ByteType>(static_cast<unsigned char>((word >> shift) & UINT64_C(0xFF)));
}

// Places a byte at bit offset shift of a 64-bit word
template <typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::uint64_t insert_byte(const ByteType value, const unsigned shift) noexcept
{
    return static_cast<std::uint64_t>(static_cast<unsigned char>(value)) << shift;
}

// Rebuilds either library type from the raw two's complement words
template <typename T>
struct word_builder;

template <>
struct word_builder<uint128>
{
    static BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 build(const std::uint64_t hi, const std::uint64_t lo) noexcept
    {
        return uint128{hi, lo};
    }
};

template <>
struct word_builder<int128>
{
    static BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 build(const std::uint64_t hi, const std::uint64_t lo) noexcept
    {
        return from_bits(hi, lo);
    }
};

// The byte arrays are built from shifts rather than from the object representation,
// so all four functions below are exact on either endianness. Bytes is anything
// that can be indexed with the subscript operator: a std::array or a pointer.

template <typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(uint128)> to_be_bytes_impl(const std::uint64_t hi, const std::uint64_t lo) noexcept
{
    return {{extract_byte<ByteType>(hi, 56U), extract_byte<ByteType>(hi, 48U),
             extract_byte<ByteType>(hi, 40U), extract_byte<ByteType>(hi, 32U),
             extract_byte<ByteType>(hi, 24U), extract_byte<ByteType>(hi, 16U),
             extract_byte<ByteType>(hi, 8U),  extract_byte<ByteType>(hi, 0U),
             extract_byte<ByteType>(lo, 56U), extract_byte<ByteType>(lo, 48U),
             extract_byte<ByteType>(lo, 40U), extract_byte<ByteType>(lo, 32U),
             extract_byte<ByteType>(lo, 24U), extract_byte<ByteType>(lo, 16U),
             extract_byte<ByteType>(lo, 8U),  extract_byte<ByteType>(lo, 0U)}};
}

template <typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(uint128)> to_le_bytes_impl(const std::uint64_t hi, const std::uint64_t lo) noexcept
{
    return {{extract_byte<ByteType>(lo, 0U),  extract_byte<ByteType>(lo, 8U),
             extract_byte<ByteType>(lo, 16U), extract_byte<ByteType>(lo, 24U),
             extract_byte<ByteType>(lo, 32U), extract_byte<ByteType>(lo, 40U),
             extract_byte<ByteType>(lo, 48U), extract_byte<ByteType>(lo, 56U),
             extract_byte<ByteType>(hi, 0U),  extract_byte<ByteType>(hi, 8U),
             extract_byte<ByteType>(hi, 16U), extract_byte<ByteType>(hi, 24U),
             extract_byte<ByteType>(hi, 32U), extract_byte<ByteType>(hi, 40U),
             extract_byte<ByteType>(hi, 48U), extract_byte<ByteType>(hi, 56U)}};
}

template <typename T, typename Bytes>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_be_bytes_impl(const Bytes& bytes) noexcept
{
    return word_builder<T>::build(insert_byte(bytes[0], 56U) | insert_byte(bytes[1], 48U) |
                                  insert_byte(bytes[2], 40U) | insert_byte(bytes[3], 32U) |
                                  insert_byte(bytes[4], 24U) | insert_byte(bytes[5], 16U) |
                                  insert_byte(bytes[6], 8U)  | insert_byte(bytes[7], 0U),
                                  insert_byte(bytes[8], 56U) | insert_byte(bytes[9], 48U) |
                                  insert_byte(bytes[10], 40U) | insert_byte(bytes[11], 32U) |
                                  insert_byte(bytes[12], 24U) | insert_byte(bytes[13], 16U) |
                                  insert_byte(bytes[14], 8U)  | insert_byte(bytes[15], 0U));
}

template <typename T, typename Bytes>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_le_bytes_impl(const Bytes& bytes) noexcept
{
    return word_builder<T>::build(insert_byte(bytes[8], 0U)  | insert_byte(bytes[9], 8U) |
                                  insert_byte(bytes[10], 16U) | insert_byte(bytes[11], 24U) |
                                  insert_byte(bytes[12], 32U) | insert_byte(bytes[13], 40U) |
                                  insert_byte(bytes[14], 48U) | insert_byte(bytes[15], 56U),
                                  insert_byte(bytes[0], 0U)  | insert_byte(bytes[1], 8U) |
                                  insert_byte(bytes[2], 16U) | insert_byte(bytes[3], 24U) |
                                  insert_byte(bytes[4], 32U) | insert_byte(bytes[5], 40U) |
                                  insert_byte(bytes[6], 48U) | insert_byte(bytes[7], 56U));
}

} // namespace detail

//=====================================
// Byte array conversions
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(uint128)> to_be_bytes(const uint128 value) noexcept
{
    return detail::to_be_bytes_impl<ByteType>(value.high, value.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(int128)> to_be_bytes(const int128 value) noexcept
{
    return detail::to_be_bytes_impl<ByteType>(value.high, value.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType, std::size_t N>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_be_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    return detail::from_be_bytes_impl<T>(bytes);
}

// Reads sizeof(T) bytes starting at bytes
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_be_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    return detail::from_be_bytes_impl<T>(bytes);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(uint128)> to_le_bytes(const uint128 value) noexcept
{
    return detail::to_le_bytes_impl<ByteType>(value.high, value.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(int128)> to_le_bytes(const int128 value) noexcept
{
    return detail::to_le_bytes_impl<ByteType>(value.high, value.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType, std::size_t N>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_le_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    return detail::from_le_bytes_impl<T>(bytes);
}

// Reads sizeof(T) bytes starting at bytes
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_le_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    return detail::from_le_bytes_impl<T>(bytes);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(uint128)> to_ne_bytes(const uint128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return detail::to_le_bytes_impl<ByteType>(value.high, value.low);

    #else

    return detail::to_be_bytes_impl<ByteType>(value.high, value.low);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::array<ByteType, sizeof(int128)> to_ne_bytes(const int128 value) noexcept
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return detail::to_le_bytes_impl<ByteType>(value.high, value.low);

    #else

    return detail::to_be_bytes_impl<ByteType>(value.high, value.low);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType, std::size_t N>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_ne_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return detail::from_le_bytes_impl<T>(bytes);

    #else

    return detail::from_be_bytes_impl<T>(bytes);

    #endif
}

// Reads sizeof(T) bytes starting at bytes
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename ByteType>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T from_ne_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int128::uint128 or boost::int128::int128");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    return detail::from_le_bytes_impl<T>(bytes);

    #else

    return detail::from_be_bytes_impl<T>(bytes);

    #endif
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_BYTE_CONVERSIONS_HPP
