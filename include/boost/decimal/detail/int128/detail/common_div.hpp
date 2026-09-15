// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_DIV_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_DIV_HPP

#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/detail/clz.hpp>
#include <boost/decimal/detail/int128/detail/common_mul.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstdint>
#include <cstring>

#endif

namespace boost {
namespace int128 {
namespace detail {

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wassume"
#endif

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void half_word_div(const T& lhs, const std::uint32_t rhs, T& quotient, T& remainder) noexcept
{
    using high_word_type = decltype(T{}.high);

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    // Use Barrett reduction-inspired approach
    const std::uint64_t divisor {rhs};

    const auto q_high {static_cast<std::uint64_t>(lhs.high) / divisor};
    auto r {static_cast<std::uint64_t>(lhs.high) % divisor};

    const auto low_high {static_cast<std::uint32_t>(lhs.low >> 32U)};
    const auto low_low {static_cast<std::uint32_t>(lhs.low)};

    r = (r << 32U) | low_high;
    const auto q_mid {r / divisor};
    r %= divisor;

    r = (r << 32U) | low_low;
    const auto q_low {r / divisor};
    r %= divisor;

    quotient.high = static_cast<high_word_type>(q_high);
    quotient.low = (q_mid << 32U) | q_low;
    remainder.low = r;
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void half_word_div(const T& lhs, const std::uint32_t rhs, T& quotient) noexcept
{
    using high_word_type = decltype(T{}.high);

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    quotient.high = static_cast<high_word_type>(static_cast<std::uint64_t>(lhs.high) / rhs);
    auto remainder {((static_cast<std::uint64_t>(lhs.high) % rhs) << 32) | (lhs.low >> 32)};
    quotient.low = (remainder / rhs) << 32;
    remainder = ((remainder % rhs) << 32) | (lhs.low & UINT32_MAX);
    quotient.low |= (remainder / rhs) & UINT32_MAX;
}

// Portable 128-bit by 64-bit unsigned division producing a 64-bit quotient and remainder.
// This is the classic Hacker's Delight divlu (two 32-bit "digit" steps over 64-bit words).
// Precondition: u1 < d so the quotient is guaranteed to fit in 64 bits. It is constexpr-safe
// and serves as the fallback for udiv_2by1 on every target without a hardware 128/64 divide.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::uint64_t divlu(std::uint64_t u1, std::uint64_t u0, std::uint64_t d, std::uint64_t& r) noexcept
{
    constexpr std::uint64_t b {UINT64_C(1) << 32U}; // Number base (2^32)

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(u1 < d); // LCOV_EXCL_LINE

    // D.1: normalize so that the divisor's most significant bit is set
    const auto s {countl_zero(d)};
    d <<= s;

    const auto vn1 {d >> 32U};
    const auto vn0 {d & UINT32_MAX};

    // Shift the dividend left by s. The (64 - s) shift is undefined when s == 0, so guard it.
    const auto un32 {s == 0 ? u1 : ((u1 << s) | (u0 >> (64 - s)))};
    const auto un10 {u0 << s};

    const auto un1 {un10 >> 32U};
    const auto un0 {un10 & UINT32_MAX};

    // First quotient digit
    auto q1 {un32 / vn1};
    auto rhat {un32 - (q1 * vn1)};

    while (q1 >= b || (q1 * vn0) > ((b * rhat) + un1))
    {
        --q1;
        rhat += vn1;
        if (rhat >= b)
        {
            break;
        }
    }

    const auto un21 {(un32 * b) + un1 - (q1 * d)};

    // Second quotient digit
    auto q0 {un21 / vn1};
    rhat = un21 - (q0 * vn1);

    while (q0 >= b || (q0 * vn0) > ((b * rhat) + un0))
    {
        --q0;
        rhat += vn1;
        if (rhat >= b)
        {
            break;
        }
    }

    // The remainder is shifted back down by the normalization amount
    r = ((un21 * b) + un0 - (q0 * d)) >> s;
    return (q1 * b) + q0;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ)

// Inline asm cannot appear in a constexpr function body before C++20, so the x86-64 DIV
// instruction is wrapped in a non-constexpr helper that udiv_2by1 only calls at runtime.
BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE std::uint64_t udiv_2by1_divq(const std::uint64_t u1, const std::uint64_t u0, const std::uint64_t d, std::uint64_t& r) noexcept
{
    std::uint64_t q {};
    __asm__("divq %[d]" : "=a"(q), "=d"(r) : [d] "r"(d), "a"(u0), "d"(u1) : "cc");
    return q;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ

// Divides the 128-bit value (u1:u0) by d, returning a 64-bit quotient and the true remainder.
// Precondition: u1 < d. Mirrors common_mul.hpp::umul: a hardware instruction at runtime where
// one exists, and the portable divlu in constexpr evaluation and everywhere else.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::uint64_t udiv_2by1(const std::uint64_t u1, const std::uint64_t u0, const std::uint64_t d, std::uint64_t& r) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_ASSUME(u1 < d); // LCOV_EXCL_LINE

    #if (defined(BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ) || (defined(_M_AMD64) && !defined(__GNUC__) && !defined(__clang__) && _MSC_VER >= 1920)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(u1))
    {
        #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ)

        return udiv_2by1_divq(u1, u0, d, r);

        #else

        return _udiv128(u1, u0, d, &r);

        #endif
    }

    #endif

    return divlu(u1, u0, d, r);
}

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127) // Pre c++17 the if constexpr remainder part will hit this
#endif

// Divides the 128-bit value (uh:ul) by the 128-bit divisor (vh:vl) where vh != 0. Because the
// divisor is >= 2^64 the quotient is guaranteed to fit in a single 64-bit word, which is
// returned. When need_remainder is true the 128-bit remainder is written to (rem_hi:rem_lo).
//
// This is one normalized quotient digit (Knuth Algorithm D specialized to a 2-word divisor).
// The top-limb estimate qhat (reusing udiv_2by1, a hardware divq on x86-64) is bounded by
// Knuth Theorem B to q <= qhat <= q + 2; the D3 refinement against d0 tightens it to q <= qhat
// <= q + 1, and the conditional add-back then corrects the remaining off-by-one.
template <bool need_remainder>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::uint64_t div3by2(const std::uint64_t uh, const std::uint64_t ul,
    const std::uint64_t vh, const std::uint64_t vl, std::uint64_t& rem_hi, std::uint64_t& rem_lo) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_ASSUME(vh != 0); // LCOV_EXCL_LINE

    // D.1: normalize so the divisor's most significant bit is set
    const auto s {countl_zero(vh)};
    const auto cs {64 - s};

    std::uint64_t d1 {};
    std::uint64_t d0 {};
    std::uint64_t u2 {};
    std::uint64_t u1 {};
    std::uint64_t u0 {};

    if (s == 0)
    {
        d1 = vh;
        d0 = vl;
        u2 = 0;
        u1 = uh;
        u0 = ul;
    }
    else
    {
        d1 = (vh << s) | (vl >> cs);
        d0 = vl << s;
        u2 = uh >> cs;
        u1 = (uh << s) | (ul >> cs);
        u0 = ul << s;
    }

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(u2 <= d1); // LCOV_EXCL_LINE

    // D.3: estimate the single quotient digit qhat = floor((u2:u1) / d1), clamped to 2^64 - 1.
    // rhat is the remainder of that estimate.
    std::uint64_t qhat {};
    std::uint64_t rhat {};
    bool rhat_overflow {false};
    if (u2 < d1)
    {
        qhat = udiv_2by1(u2, u1, d1, rhat);
    }
    else
    {
        // u2 == d1: floor((u2:u1)/d1) clamps to 2^64 - 1, leaving rhat == u1 + d1 (may carry).
        qhat = UINT64_MAX;
        rhat = u1 + d1;
        rhat_overflow = rhat < u1;
    }

    std::uint64_t qd0_hi {};
    auto qd0_lo {umul(qhat, d0, qd0_hi)};

    // Refine qhat against d0 (Knuth D3). The top-limb estimate alone can exceed the true quotient
    // by up to 2; this brings it down to at most one too large, which the add-back below corrects.
    // At most two iterations run, and only while the running remainder rhat stays below 2^64.
    if (!rhat_overflow)
    {
        while (qd0_hi > rhat || (qd0_hi == rhat && qd0_lo > u0))
        {
            --qhat;
            rhat += d1;
            const auto rhat_carry {rhat < d1};
            qd0_lo = umul(qhat, d0, qd0_hi);
            if (rhat_carry)
            {
                break;
            }
        }
    }

    // D.4: multiply and subtract (u2:u1:u0) - qhat * (d1:d0). qd0 already holds qhat * d0.
    std::uint64_t qd1_hi {};
    const auto qd1_lo {umul(qhat, d1, qd1_hi)};

    const auto p0 {qd0_lo};
    const auto p1 {qd0_hi + qd1_lo};
    const auto p2 {qd1_hi + static_cast<std::uint64_t>(p1 < qd0_hi)};

    const auto r0 {u0 - p0};
    const auto borrow0 {static_cast<std::uint64_t>(u0 < p0)};
    const auto t1 {u1 - p1};
    auto r1 {t1 - borrow0};
    const auto borrow1 {static_cast<std::uint64_t>(u1 < p1) + static_cast<std::uint64_t>(t1 < borrow0)};

    // D.5/D.6: if the top limb borrowed, qhat was one too large. Correct it and add the divisor
    // back into the remainder. The probability of this branch is small.
    auto r0_final {r0};
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY((u2 < p2) || ((u2 - p2) < borrow1)))
    {
        --qhat;                                                  // LCOV_EXCL_LINE
        const auto sum0 {r0 + d0};                               // LCOV_EXCL_LINE
        r0_final = sum0;                                         // LCOV_EXCL_LINE
        r1 = r1 + d1 + static_cast<std::uint64_t>(sum0 < r0);    // LCOV_EXCL_LINE
    }

    BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (need_remainder)
    {
        if (s == 0)
        {
            rem_hi = r1;
            rem_lo = r0_final;
        }
        else
        {
            rem_lo = (r0_final >> s) | (r1 << cs);
            rem_hi = r1 >> s;
        }
    }
    else
    {
        static_cast<void>(rem_hi);
        static_cast<void>(rem_lo);
    }

    return qhat;
}

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

namespace impl {

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127) // Pre c++17 the if constexpr remainder part will hit this
#endif

template <std::size_t v_size>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void unpack_v(std::uint32_t (&vn)[4], const std::uint32_t (&v)[v_size],
    const bool needs_shift, const int s, const int complement_s, const std::integral_constant<std::size_t, 2>&) noexcept
{
    vn[1] = needs_shift ? ((v[1] << s) | (v[0] >> complement_s)) : v[1];
    vn[0] = needs_shift ? (v[0] << s) : v[0];
}

template <std::size_t v_size>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void unpack_v(std::uint32_t (&vn)[4], const std::uint32_t (&v)[v_size],
    const bool needs_shift, const int s, const int complement_s, const std::integral_constant<std::size_t, 4>&) noexcept
{
    vn[3] = needs_shift ? ((v[3] << s) | (v[2] >> complement_s)) : v[3];
    vn[2] = needs_shift ? ((v[2] << s) | (v[1] >> complement_s)) : v[2];
    vn[1] = needs_shift ? ((v[1] << s) | (v[0] >> complement_s)) : v[1];
    vn[0] = needs_shift ? (v[0] << s) : v[0];
}

// See: The Art of Computer Programming Volume 2 (Semi-numerical algorithms) section 4.3.1
// Algorithm D: Division of Non-negative integers
template <bool need_remainder, std::size_t u_size, std::size_t v_size, std::size_t q_size>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr void knuth_divide(std::uint32_t (&u)[u_size], const std::size_t m,
                            const std::uint32_t (&v)[v_size], const std::size_t n,
                            std::uint32_t (&q)[q_size]) noexcept
{
    // D.1
    const auto s {countl_zero(v[n - 1])};
    const auto complement_s {32 - s};
    const bool needs_shift {s > 0};

    // Create normalized versions of u and v
    std::uint32_t un[5] {};
    std::uint32_t vn[4] {};

    un[4] = needs_shift ? (u[3] >> complement_s) : 0;
    un[3] = needs_shift ? ((u[3] << s) | (u[2] >> complement_s)) : u[3];
    un[2] = needs_shift ? ((u[2] << s) | (u[1] >> complement_s)) : u[2];
    un[1] = needs_shift ? ((u[1] << s) | (u[0] >> complement_s)) : u[1];
    un[0] = needs_shift ? (u[0] << s) : u[0];

    static_assert(v_size == 4 || v_size == 2, "Unknown size for denominator");
    unpack_v(vn, v, needs_shift, s, complement_s, std::integral_constant<std::size_t, v_size>{});

    // D.2
    for (std::size_t j {m - n}; j != static_cast<std::size_t>(-1); --j)
    {
        // D.3
        const auto dividend {(static_cast<std::uint64_t>(un[j+n]) << 32) | un[j+n-1]};
        const auto divisor {static_cast<std::uint64_t>(vn[n-1])};
        auto q_hat {dividend / divisor};
        auto r_hat {dividend % divisor};

        while (q_hat > UINT32_MAX ||
               (q_hat * vn[n-2]) > ((r_hat << 32) | un[j+n-2]))
        {
            --q_hat;
            r_hat += vn[n-1];
            if (r_hat > UINT32_MAX)
            {
                break;
            }
        }

        // D.4
        std::int64_t borrow {};
        for (std::size_t i {}; i < n; ++i)
        {
            const auto p {q_hat * vn[i]};
            const auto p_lo {static_cast<std::uint32_t>(p & UINT32_MAX)};
            const auto p_hi {static_cast<std::uint32_t>(p >> 32)};

            borrow += static_cast<std::int64_t>(un[j+i]) - static_cast<std::int64_t>(p_lo);
            un[j+i] = static_cast<std::uint32_t>(borrow & UINT32_MAX);
            borrow >>= 32;

            borrow -= p_hi;
        }
        borrow += un[j+n];
        un[j+n] = static_cast<std::uint32_t>(borrow & UINT32_MAX);

        // D.5
        q[j] = static_cast<std::uint32_t>(q_hat & UINT32_MAX);
        if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(borrow < 0))
        {
            // D.6
            // The probability of hitting this path is about 4.7e-10
            --q[j];                                                             // LCOV_EXCL_LINE
            std::uint64_t carry {};                                             // LCOV_EXCL_LINE
            for (std::size_t i = 0; i < n; ++i)                                 // LCOV_EXCL_LINE
            {                                                                   // LCOV_EXCL_LINE
                carry += static_cast<std::uint64_t>(un[j+i]) + vn[i];           // LCOV_EXCL_LINE
                un[j+i] = static_cast<std::uint32_t>(carry & UINT32_MAX);       // LCOV_EXCL_LINE
                carry >>= 32U;                                                  // LCOV_EXCL_LINE
            }                                                                   // LCOV_EXCL_LINE
            un[j+n] += static_cast<std::uint32_t>(carry & UINT32_MAX);          // LCOV_EXCL_LINE
        }
    }

    // D.8
    // If we are only calculating division we can completely skip this step
    BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (need_remainder)
    {
        if (s > 0)
        {
            for (std::size_t i {}; i < n-1; i++)
            {
                u[i] = (un[i] >> s) | (un[i+1] << (32 - s));
            }
            u[n-1] = un[n-1] >> s;
        }
        else
        {
            for (std::size_t i {}; i < n; i++)
            {
                u[i] = un[i];
            }
        }

        // Clear anything left in u
        for (std::size_t i {n}; i < m; i++)
        {
            u[i] = 0;
        }
    }
}

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::size_t to_words(const T& x, std::uint32_t (&words)[4]) noexcept
{
    #if !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && !BOOST_DECIMAL_DETAIL_INT128_ENDIAN_BIG_BYTE
    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        std::memcpy(&words, &x, sizeof(T));
    }
    else
    #endif
    {
        words[0] = static_cast<std::uint32_t>(x.low & UINT32_MAX);                              // LCOV_EXCL_LINE
        words[1] = static_cast<std::uint32_t>(x.low >> 32);                                     // LCOV_EXCL_LINE
        words[2] = static_cast<std::uint32_t>(static_cast<std::uint64_t>(x.high) & UINT32_MAX); // LCOV_EXCL_LINE
        words[3] = static_cast<std::uint32_t>(static_cast<std::uint64_t>(x.high) >> 32);        // LCOV_EXCL_LINE
    }

    BOOST_DECIMAL_DETAIL_INT128_ASSERT_MSG(x != static_cast<T>(0), "Division by 0");

    std::size_t word_count {4};
    while (words[word_count - 1U] == 0U)
    {
        word_count--;
    }

    return word_count;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::size_t to_words(const std::uint64_t x, std::uint32_t (&words)[2]) noexcept
{
    #if !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && !BOOST_DECIMAL_DETAIL_INT128_ENDIAN_BIG_BYTE
    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(x))
    {
        std::memcpy(&words, &x, sizeof(std::uint64_t));
    }
    else
    #endif
    {
        words[0] = static_cast<std::uint32_t>(x & UINT32_MAX);  // LCOV_EXCL_LINE
        words[1] = static_cast<std::uint32_t>(x >> 32);         // LCOV_EXCL_LINE
    }

    return x > UINT32_MAX ? 2 : 1;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr std::size_t to_words(const std::uint32_t x, std::uint32_t (&words)[1]) noexcept
{
    words[0] = x;

    return 1;
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr T from_words(const std::uint32_t (&words)[4]) noexcept
{
    const auto low {static_cast<std::uint64_t>(words[0]) | (static_cast<std::uint64_t>(words[1]) << 32)};
    const auto high {static_cast<std::uint64_t>(words[2]) | (static_cast<std::uint64_t>(words[3]) << 32)};

    return {static_cast<ctor_high_word_t<T>>(high), low};
}

} // namespace impl

// We only need to take the time to process the remainder in the modulo case
// In the division case it is a waste of cycles
//
// 128/64 -> 128-bit quotient (and optional 64-bit remainder) by two-step long division.
// The leading 64/64 yields the high quotient word and a remainder r < rhs, which satisfies
// the udiv_2by1 precondition for the low quotient word. This covers every rhs (including
// rhs <= UINT32_MAX) through the single hardware-or-portable udiv_2by1 primitive.

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void one_word_div(const T& lhs, const std::uint64_t rhs, T& quotient) noexcept
{
    using high_word_type = decltype(T{}.high);

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    const auto u_high {static_cast<std::uint64_t>(lhs.high)};

    quotient.high = static_cast<high_word_type>(u_high / rhs);
    auto r {u_high % rhs};
    quotient.low = udiv_2by1(r, lhs.low, rhs, r);
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void one_word_div(const T& lhs, const std::uint64_t rhs, T& quotient, T& remainder) noexcept
{
    using high_word_type = decltype(T{}.high);

    BOOST_DECIMAL_DETAIL_INT128_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    const auto u_high {static_cast<std::uint64_t>(lhs.high)};

    quotient.high = static_cast<high_word_type>(u_high / rhs);
    auto r {u_high % rhs};
    quotient.low = udiv_2by1(r, lhs.low, rhs, r);

    remainder.high = static_cast<high_word_type>(0);
    remainder.low = r;
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void one_word_div(const T& lhs, const std::uint32_t rhs, T& quotient, T& remainder) noexcept
{
    one_word_div(lhs, static_cast<std::uint64_t>(rhs), quotient, remainder);
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr void one_word_div(const T& lhs, const std::uint32_t rhs, T& quotient) noexcept
{
    one_word_div(lhs, static_cast<std::uint64_t>(rhs), quotient);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant is true pre-C++17
#  pragma warning(disable : 4804) // Unsafe comparison with bool
#endif

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr T knuth_div(const T& dividend, const T& divisor) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_ASSUME(divisor != static_cast<T>(0));

    std::uint64_t rem_hi {};
    std::uint64_t rem_lo {};

    const auto q {div3by2<false>(static_cast<std::uint64_t>(dividend.high), dividend.low,
                                 static_cast<std::uint64_t>(divisor.high), divisor.low, rem_hi, rem_lo)};

    return T{static_cast<ctor_high_word_t<T>>(0), q};
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr T knuth_div(const T& dividend, const T& divisor, T& remainder) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_ASSUME(divisor != static_cast<T>(0));

    std::uint64_t rem_hi {};
    std::uint64_t rem_lo {};

    const auto q {div3by2<true>(static_cast<std::uint64_t>(dividend.high), dividend.low,
                                static_cast<std::uint64_t>(divisor.high), divisor.low, rem_hi, rem_lo)};

    remainder = T{static_cast<ctor_high_word_t<T>>(rem_hi), rem_lo};

    return T{static_cast<ctor_high_word_t<T>>(0), q};
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif


} // namespace detail
} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_COMMON_DIV_HPP
