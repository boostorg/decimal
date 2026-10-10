// Copyright 2023 - 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_DIV_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_DIV_IMPL_HPP

#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/add_impl.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/normalize.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/components.hpp>
#include <boost/decimal/detail/u256.hpp>
#include "int128.hpp"

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant (pre-C++17 if-constexpr fallback)
#endif

namespace impl {

// Default-rounding gate. The inline round-half-to-even (RHTE) in
// div_finalize_u64 assumes the active rounding mode is
// fe_dec_to_nearest. Other modes route through the constructor handoff
// (which dispatches to fenv_round honoring the runtime mode).
template <typename Anchor>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto div_default_rounding(const Anchor& anchor) noexcept -> bool
{
    static_cast<void>(anchor);
    bool default_rounding {_boost_decimal_global_rounding_mode == rounding_mode::fe_dec_to_nearest};
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    if (!BOOST_DECIMAL_IS_CONSTANT_EVALUATED(anchor))
    {
        default_rounding = (_boost_decimal_global_runtime_rounding_mode == rounding_mode::fe_dec_to_nearest);
    }
    #endif
    return default_rounding;
}

// Division finalization with inline round-half-to-even. The 64-bit and 128-bit finalizers
// round in the mode that the caller gives, with this rule for fe_dec_to_nearest. The driver computes
// q = (lhs.sig * 10^k) / rhs.sig and r = (lhs.sig * 10^k) % rhs.sig where
// both operand significands are at full precision p, and k is p - 1 when
// lhs.sig >= rhs.sig, else p. Thus q has exactly p digits, and the natural
// remainder r encodes the exact sticky bit needed for correct RHTE.
//
// Round up iff
//   2r > divisor                            (computed as r > divisor - r to
//                                            avoid overflow when divisor is
//                                            near the type's max)
//   OR (2r == divisor AND q is odd).
//
// The post-rounding carry q == 10^p shifts down to 10^(p-1) and bumps the
// exponent by one digit.

// d32/fast32 finalizer. Dividend = lhs.sig * 10^7 fits in uint64; quotient is
// at most 10^7 - 1 which fits in uint32; remainder is bounded by the divisor
// which fits in uint32 (rhs.sig < 10^7).
template <typename ReturnType, typename ExpType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto div_finalize_u64(
    std::uint64_t q, std::uint64_t r, std::uint32_t divisor,
    ExpType result_exp, bool sign) noexcept -> ReturnType
{
    constexpr auto ten_to_p {pow10(static_cast<std::uint64_t>(detail::precision_v<ReturnType>))};
    constexpr auto ten_to_p_minus_1 {pow10(static_cast<std::uint64_t>(detail::precision_v<ReturnType> - 1))};

    // Below the subnormal exponent the constructor rounds once, with the remainder as a sticky digit.
    // Rounding to the precision here first would round the result twice.
    if (!detail::is_fast_type_v<ReturnType> && BOOST_DECIMAL_UNLIKELY(result_exp < detail::etiny_v<ReturnType>))
    {
        return pack_in_range<ReturnType>(static_cast<typename ReturnType::significand_type>(q * 10U + static_cast<std::uint64_t>(r != 0U)), result_exp - 1, sign);
    }

    int extra {0};

    const auto half_div {static_cast<std::uint64_t>(divisor) - r};
    const bool round_up {r > half_div || (r == half_div && (q & UINT64_C(1)) != UINT64_C(0))};
    if (round_up)
    {
        ++q;
        if (BOOST_DECIMAL_UNLIKELY(q == ten_to_p))
        {
            q = ten_to_p_minus_1;
            ++extra;
        }
    }

    using sig_type = typename ReturnType::significand_type;
    return pack_in_range<ReturnType>(static_cast<sig_type>(q),
                                     result_exp + static_cast<ExpType>(extra),
                                     sign);
}

// d64/fast64 finalizer. Quotient and remainder both fit in uint64 because
// quotient is at most 10^16 - 1 < 2^54 and remainder is bounded by the
// uint64 divisor.
template <typename ReturnType, typename ExpType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto div_finalize_u128(
    std::uint64_t q, std::uint64_t r, std::uint64_t divisor,
    ExpType result_exp, bool sign, const rounding_mode round) noexcept -> ReturnType
{
    constexpr auto ten_to_p {pow10(static_cast<std::uint64_t>(detail::precision_v<ReturnType>))};
    constexpr auto ten_to_p_minus_1 {pow10(static_cast<std::uint64_t>(detail::precision_v<ReturnType> - 1))};

    // Below the subnormal exponent the constructor rounds once, with the remainder as a sticky digit.
    // Rounding to the precision here first would round the result twice.
    if (!detail::is_fast_type_v<ReturnType> && BOOST_DECIMAL_UNLIKELY(result_exp < detail::etiny_v<ReturnType>))
    {
        return pack_in_range<ReturnType>(static_cast<typename ReturnType::significand_type>(q * 10U + static_cast<std::uint64_t>(r != 0U)), result_exp - 1, sign);
    }

    int extra {0};

    const auto half_div {divisor - r};
    const bool round_up {round == rounding_mode::fe_dec_to_nearest ?
                         r > half_div || (r == half_div && (q & UINT64_C(1)) != UINT64_C(0)) :
                         detail::steps_up_in_mode(round, sign, r != 0U, r >= half_div)};
    if (round_up)
    {
        ++q;
        if (BOOST_DECIMAL_UNLIKELY(q == ten_to_p))
        {
            q = ten_to_p_minus_1;
            ++extra;
        }
    }

    using sig_type = typename ReturnType::significand_type;
    return pack_in_range<ReturnType>(static_cast<sig_type>(q),
                                     result_exp + static_cast<ExpType>(extra),
                                     sign);
}

// d128/fast128 finalizer. Quotient and remainder both fit in uint128 because
// quotient is at most 10^34 - 1 (< 2^113) and remainder is bounded by the
// uint128 divisor (< 10^34).
template <typename ReturnType, typename ExpType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto div_finalize_u256(
    int128::uint128_t q, int128::uint128_t r, int128::uint128_t divisor,
    ExpType result_exp, bool sign, const rounding_mode round) noexcept -> ReturnType
{
    constexpr auto ten_to_p {pow10(int128::uint128_t{static_cast<std::uint64_t>(detail::precision_v<ReturnType>)})};
    constexpr auto ten_to_p_minus_1 {pow10(int128::uint128_t{static_cast<std::uint64_t>(detail::precision_v<ReturnType> - 1)})};

    // Below the subnormal exponent the constructor rounds once, with the remainder as a sticky digit.
    // Rounding to the precision here first would round the result twice.
    if (!detail::is_fast_type_v<ReturnType> && BOOST_DECIMAL_UNLIKELY(result_exp < detail::etiny_v<ReturnType>))
    {
        return pack_in_range<ReturnType>(static_cast<typename ReturnType::significand_type>(q * 10U + static_cast<std::uint64_t>(r != int128::uint128_t{0U})), result_exp - 1, sign);
    }

    int extra {0};

    const auto half_div {divisor - r};
    const bool round_up {round == rounding_mode::fe_dec_to_nearest ?
                         r > half_div || (r == half_div && (q.low & UINT64_C(1)) != UINT64_C(0)) :
                         detail::steps_up_in_mode(round, sign, r != 0U, r >= half_div)};
    if (round_up)
    {
        ++q;
        if (BOOST_DECIMAL_UNLIKELY(q == ten_to_p))
        {
            q = ten_to_p_minus_1;
            ++extra;
        }
    }

    return pack_in_range<ReturnType>(q,
                                     result_exp + static_cast<ExpType>(extra),
                                     sign);
}

} // namespace impl

// d32/fast32 division driver. Accepts a decimal type or a components struct
// (both expose to_components()) and dispatches to the inline RHTE fast path
// when the active rounding mode is fe_dec_to_nearest. Non-default rounding
// modes fall back to the original wide-divide + constructor handoff so the
// constructor's coefficient_rounding can dispatch to fenv_round.
template <typename DecimalType, typename T>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto generic_div_impl(const T& lhs, const T& rhs) noexcept -> DecimalType
{
    auto lhs_c {lhs.to_components()};
    auto rhs_c {rhs.to_components()};

    detail::expand_significand<DecimalType>(lhs_c.sig, lhs_c.exp);
    detail::expand_significand<DecimalType>(rhs_c.sig, rhs_c.exp);

    const bool sign {lhs_c.sign != rhs_c.sign};

    if (BOOST_DECIMAL_UNLIKELY(lhs_c.sig == 0U))
    {
        using sig_type = typename decltype(lhs_c)::significand_type;
        return DecimalType{sig_type{0U}, lhs_c.exp - rhs_c.exp, sign};
    }

    if (BOOST_DECIMAL_UNLIKELY(!impl::div_default_rounding(lhs_c.sig)))
    {
        // Non-default rounding mode: hand the wide quotient to the
        // constructor's coefficient_rounding so fenv_round honors the
        // runtime mode. The wider offset preserves the original behavior.
        constexpr auto wide_offset {std::numeric_limits<std::uint64_t>::digits10 - precision_v<DecimalType>};
        constexpr auto wide_tens {pow10(static_cast<std::uint64_t>(wide_offset))};
        const auto wide_sig {static_cast<std::uint64_t>(lhs_c.sig) * wide_tens};
        const auto wide_div {static_cast<std::uint64_t>(rhs_c.sig)};
        const auto wide_q {wide_sig / wide_div};
        // The constructor rounds by the digits it drops, thus the remainder goes in as one digit
        const auto sticky {static_cast<unsigned>(wide_sig - wide_q * wide_div != 0U)};
        const auto wide_exp {(lhs_c.exp - static_cast<int>(wide_offset) - 1) - rhs_c.exp};
        return DecimalType{wide_q * 10U + sticky, wide_exp, sign};
    }

    // Scale by 10^(p-1) when lhs >= rhs, thus the quotient has exactly p digits
    constexpr auto ten_to_p {pow10(static_cast<std::uint64_t>(precision_v<DecimalType>))};
    const bool shorter {lhs_c.sig >= rhs_c.sig};
    const auto big_sig {static_cast<std::uint64_t>(lhs_c.sig) * (shorter ? ten_to_p / 10U : ten_to_p)};
    const auto divisor {static_cast<std::uint64_t>(rhs_c.sig)};
    const auto q {big_sig / divisor};
    const auto r {big_sig - q * divisor};
    const auto res_exp {(lhs_c.exp - static_cast<int>(precision_v<DecimalType>)) - rhs_c.exp + static_cast<int>(shorter)};

    return impl::div_finalize_u64<DecimalType>(q, r, static_cast<std::uint32_t>(divisor), res_exp, sign);
}

// d64/fast64 division driver. Same structure as the d32 driver but uses
// uint128 for the pre-scaled dividend (lhs.sig * 10^16 overflows uint64).
// The quotient is at most 10^16 - 1 < 2^54 so it narrows safely to uint64
// for the finalizer.
template <typename DecimalType, typename T>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto d64_generic_div_impl(const T& lhs, const T& rhs, const bool sign) noexcept -> DecimalType
{
    using unsigned_int128_type = boost::int128::uint128_t;

    auto lhs_c {lhs.to_components()};
    auto rhs_c {rhs.to_components()};

    detail::expand_significand<DecimalType>(lhs_c.sig, lhs_c.exp);
    detail::expand_significand<DecimalType>(rhs_c.sig, rhs_c.exp);

    if (BOOST_DECIMAL_UNLIKELY(lhs_c.sig == 0U))
    {
        using sig_type = typename decltype(lhs_c)::significand_type;
        return DecimalType{sig_type{0U}, lhs_c.exp - rhs_c.exp, sign};
    }

    // Scale by 10^(p-1) when lhs >= rhs, thus the quotient has exactly p digits
    constexpr auto ten_to_p {pow10(static_cast<std::uint64_t>(precision_v<DecimalType>))};
    const bool shorter {lhs_c.sig >= rhs_c.sig};
    const auto big_sig {static_cast<unsigned_int128_type>(lhs_c.sig) * (shorter ? ten_to_p / 10U : ten_to_p)};
    const auto divisor {static_cast<std::uint64_t>(rhs_c.sig)};
    const auto q_wide {big_sig / static_cast<unsigned_int128_type>(divisor)};
    const auto r_wide {big_sig - q_wide * static_cast<unsigned_int128_type>(divisor)};
    const auto res_exp {(lhs_c.exp - static_cast<int>(precision_v<DecimalType>)) - rhs_c.exp + static_cast<int>(shorter)};

    return impl::div_finalize_u128<DecimalType>(static_cast<std::uint64_t>(q_wide.low),
                                                static_cast<std::uint64_t>(r_wide.low),
                                                divisor,
                                                res_exp,
                                                sign,
                                                detail::current_rounding_mode());
}

// d128/fast128 division driver. The pre-scaled dividend lhs.sig * 10^34
// needs u256 because the product may reach ~10^68 (well above uint128 max).
// The quotient itself fits in uint128 (< 10^34) so we narrow before the
// finalizer.
template <typename DecimalType, typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto d128_generic_div_impl(const T& lhs, const T& rhs, const bool sign) noexcept -> DecimalType
{
    auto lhs_c {lhs.to_components()};
    auto rhs_c {rhs.to_components()};

    detail::expand_significand<DecimalType>(lhs_c.sig, lhs_c.exp);
    detail::expand_significand<DecimalType>(rhs_c.sig, rhs_c.exp);

    if (BOOST_DECIMAL_UNLIKELY(lhs_c.sig == int128::uint128_t{0U}))
    {
        return DecimalType{int128::uint128_t{0U}, lhs_c.exp - rhs_c.exp, sign};
    }

    // Scale by 10^(p-1) when lhs >= rhs, thus the quotient has exactly p digits
    constexpr auto ten_to_p {pow10(int128::uint128_t{static_cast<std::uint64_t>(precision_v<DecimalType>)})};
    constexpr auto ten_to_p_minus_1 {pow10(int128::uint128_t{static_cast<std::uint64_t>(precision_v<DecimalType> - 1)})};
    const bool shorter {lhs_c.sig >= rhs_c.sig};
    const auto big_sig {detail::umul256(lhs_c.sig, shorter ? ten_to_p_minus_1 : ten_to_p)};
    const auto divisor {rhs_c.sig};
    const auto dr {impl::div_mod(big_sig, divisor)};

    const int128::uint128_t q {dr.quotient.bytes[1], dr.quotient.bytes[0]};
    const int128::uint128_t r {dr.remainder.bytes[1], dr.remainder.bytes[0]};
    const auto res_exp {lhs_c.exp - rhs_c.exp - static_cast<int>(precision_v<DecimalType>) + static_cast<int>(shorter)};

    return impl::div_finalize_u256<DecimalType>(q, r, divisor, res_exp, sign, detail::current_rounding_mode());
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

} // namespace detail
} // namespace decimal
} // namespace boost

#endif //BOOST_DECIMAL_DETAIL_DIV_IMPL_HPP
