// Copyright 2024 Junekey Jeon
// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/jk-jeon/rtz_benchmark/tree/master

#ifndef BOOST_DECIMAL_DETAIL_REMOVE_TRAILING_ZEROS_HPP
#define BOOST_DECIMAL_DETAIL_REMOVE_TRAILING_ZEROS_HPP

#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include "int128.hpp"

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

// n is assumed to be at most of bit_width bits.
template <std::size_t bit_width, typename UInt>
constexpr auto rotr(UInt n, unsigned int r) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_unsigned_v, UInt)
{
    r &= (bit_width - 1);
    return (n >> r) | (n << ((bit_width - r) & (bit_width - 1)));
}

template <typename T>
struct remove_trailing_zeros_return
{
    T trimmed_number;
    std::size_t number_of_removed_zeros;
};

constexpr auto remove_trailing_zeros(std::uint32_t n) noexcept -> remove_trailing_zeros_return<std::uint32_t>
{
    std::size_t s {};

    auto r = rotr<32>(n * UINT32_C(15273505), 8);
    auto b = r < UINT32_C(43);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<32>(n * UINT32_C(184254097), 4);
    b = r < UINT32_C(429497);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<32>(n * UINT32_C(42949673), 2);
    b = r < UINT32_C(42949673);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<32>(n * UINT32_C(1288490189), 1);
    b = r < UINT32_C(429496730);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    return {n, s};
}

constexpr auto remove_trailing_zeros(std::uint64_t n) noexcept -> remove_trailing_zeros_return<std::uint64_t>
{
    std::size_t s {};

    auto r = rotr<64>(n * UINT64_C(230079197716545), 16);
    auto b = r < UINT64_C(1845);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<64>(n * UINT64_C(28999941890838049), 8);
    b = r < UINT64_C(184467440738);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<64>(n * UINT64_C(182622766329724561), 4);
    b = r < UINT64_C(1844674407370956);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<64>(n * UINT64_C(10330176681277348905), 2);
    b = r < UINT64_C(184467440737095517);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<64>(n * UINT64_C(14757395258967641293), 1);
    b = r < UINT64_C(1844674407370955162);
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    return {n, s};
}

constexpr auto remove_trailing_zeros(boost::int128::uint128_t n) noexcept -> remove_trailing_zeros_return<boost::int128::uint128_t>
{
    std::size_t s {};

    auto r = rotr<128>(n * boost::int128::uint128_t(UINT64_C(0x62B42691AD836EB1), UINT64_C(0x16590F420A835081)), 32);
    auto b = r < boost::int128::uint128_t {UINT64_C(0x0), UINT64_C(0x33EC48)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * boost::int128::uint128_t {UINT64_C(0x3275305C1066), UINT64_C(0xE4A4D1417CD9A041)}, 16);
    b = r < boost::int128::uint128_t {UINT64_C(0x734), UINT64_C(0xACA5F6226F0ADA62)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * boost::int128::uint128_t {UINT64_C(0x6B7213EE9F5A78), UINT64_C(0xC767074B22E90E21)}, 8);
    b = r < boost::int128::uint128_t {UINT64_C(0x2AF31DC461), UINT64_C(0x1873BF3F70834ACE)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * boost::int128::uint128_t {UINT64_C(0x95182A9930BE0DE), UINT64_C(0xD288CE703AFB7E91)}, 4);
    b = r < boost::int128::uint128_t {UINT64_C(0x68DB8BAC710CB), UINT64_C(0x295E9E1B089A0276)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * boost::int128::uint128_t {UINT64_C(0x28F5C28F5C28F5C2), UINT64_C(0x8F5C28F5C28F5C29)}, 2);
    b = r < boost::int128::uint128_t {UINT64_C(0x28F5C28F5C28F5C), UINT64_C(0x28F5C28F5C28F5C3)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * boost::int128::uint128_t {UINT64_C(0xCCCCCCCCCCCCCCCC), UINT64_C(0xCCCCCCCCCCCCCCCD)}, 1);
    b = r < boost::int128::uint128_t {UINT64_C(0x1999999999999999), UINT64_C(0x999999999999999A)};
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    return {n, s};
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

constexpr auto remove_trailing_zeros(boost::int128::detail::builtin_u128 n) noexcept -> remove_trailing_zeros_return<boost::int128::uint128_t>
{
    using u128 = boost::int128::detail::builtin_u128;

    std::size_t s {};

    auto r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t(UINT64_C(0x62B42691AD836EB1), UINT64_C(0x16590F420A835081))), 32);
    auto b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x0), UINT64_C(0x33EC48)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t{UINT64_C(0x3275305C1066), UINT64_C(0xE4A4D1417CD9A041)}), 16);
    b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x734), UINT64_C(0xACA5F6226F0ADA62)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x6B7213EE9F5A78), UINT64_C(0xC767074B22E90E21)}), 8);
    b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x2AF31DC461), UINT64_C(0x1873BF3F70834ACE)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x95182A9930BE0DE), UINT64_C(0xD288CE703AFB7E91)}), 4);
    b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x68DB8BAC710CB), UINT64_C(0x295E9E1B089A0276)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x28F5C28F5C28F5C2), UINT64_C(0x8F5C28F5C28F5C29)}), 2);
    b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x28F5C28F5C28F5C), UINT64_C(0x28F5C28F5C28F5C3)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    r = rotr<128>(n * static_cast<u128>(boost::int128::uint128_t {UINT64_C(0xCCCCCCCCCCCCCCCC), UINT64_C(0xCCCCCCCCCCCCCCCD)}), 1);
    b = r < static_cast<u128>(boost::int128::uint128_t {UINT64_C(0x1999999999999999), UINT64_C(0x999999999999999A)});
    s = s * 2U + static_cast<std::size_t>(b);
    n = b ? r : n;

    return {n, s};
}

#endif

namespace patch {

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4127) // Unreachable code
#endif

template <typename T>
constexpr auto is_pure_p10(const typename T::significand_type& gn, int* p_removed_zeros) noexcept -> bool
{
    const auto
        zeros_removal
        {
            remove_trailing_zeros(gn)
        };

    bool is_pure { static_cast<int>(zeros_removal.trimmed_number) == 1 };

    BOOST_DECIMAL_IF_CONSTEXPR(std::numeric_limits<typename T::significand_type>::digits > 64)
    {
        // Catch a rare overflow case for 128-bit decimal types when counting trailing zeros and
        // patch it here. See also Git issue: https://github.com/cppalliance/decimal/issues/1110

        if(is_pure)
        {
            // Scale the argument to the interval 1 <= x < 10.
            // Scaling is needed to check floor-argument equality.
            const T gx { gn, -std::numeric_limits<T>::digits10 + 1 };

            if(floor(gx) != gx)
            {
                is_pure = false;
            }
        }
    }

    if(p_removed_zeros != nullptr)
    {
        *p_removed_zeros = static_cast<int>(zeros_removal.number_of_removed_zeros);
    }

    return is_pure;
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // namespace patch


} // namespace detail
} // namespace decimal
} // namespace boost

#endif //BOOST_DECIMAL_DETAIL_REMOVE_TRAILING_ZEROS_HPP
