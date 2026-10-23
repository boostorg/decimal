// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/construction_sign.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/int128.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

namespace fixed_point_detail {

template <bool b>
struct fixed_point_table_imp
{
    // q62_recip[i] = floor(2^(62 + s) / 10^k) with k = i + 7 and s = q62_shift[i]; the Q62 raw
    // value of t = sig * 10^-k is then (sig * q62_recip[i]) >> s, with no division
    static constexpr std::array<std::uint64_t, 28> q62_recip = {{
        UINT64_C(15474250491067253436),
        UINT64_C(12379400392853802748),
        UINT64_C(9903520314283042199),
        UINT64_C(15845632502852867518),
        UINT64_C(12676506002282294014),
        UINT64_C(10141204801825835211),
        UINT64_C(16225927682921336339),
        UINT64_C(12980742146337069071),
        UINT64_C(10384593717069655257),
        UINT64_C(16615349947311448411),
        UINT64_C(13292279957849158729),
        UINT64_C(10633823966279326983),
        UINT64_C(17014118346046923173),
        UINT64_C(13611294676837538538),
        UINT64_C(10889035741470030830),
        UINT64_C(17422457186352049329),
        UINT64_C(13937965749081639463),
        UINT64_C(11150372599265311570),
        UINT64_C(17840596158824498513),
        UINT64_C(14272476927059598810),
        UINT64_C(11417981541647679048),
        UINT64_C(18268770466636286477),
        UINT64_C(14615016373309029182),
        UINT64_C(11692013098647223345),
        UINT64_C(9353610478917778676),
        UINT64_C(14965776766268445882),
        UINT64_C(11972621413014756705),
        UINT64_C(9578097130411805364)
    }};
    static constexpr std::array<int, 28> q62_shift = {{25, 28, 31, 35, 38, 41, 45, 48, 51, 55, 58, 61, 65, 68, 71, 75, 78, 81, 85, 88, 91, 95, 98, 101, 104, 108, 111, 114}};

    // q126_recip[i] = floor(2^(126 + s) / 10^k) with k = i + 34 and s = q126_shift[i]; the Q126
    // raw value of t = sig * 10^-k is then (sig * q126_recip[i]) >> s, with no division
    static constexpr std::array<int128::uint128_t, 38> q126_recip = {{
        int128::uint128_t {UINT64_C(9578097130411805364), UINT64_C(13644483260788183358)},
        int128::uint128_t {UINT64_C(15324955408658888583), UINT64_C(10763126773035362404)},
        int128::uint128_t {UINT64_C(12259964326927110866), UINT64_C(15989199047912110569)},
        int128::uint128_t {UINT64_C(9807971461541688693), UINT64_C(9102010423587778132)},
        int128::uint128_t {UINT64_C(15692754338466701909), UINT64_C(10873867862998534689)},
        int128::uint128_t {UINT64_C(12554203470773361527), UINT64_C(12388443105140738074)},
        int128::uint128_t {UINT64_C(10043362776618689222), UINT64_C(2532056854628769813)},
        int128::uint128_t {UINT64_C(16069380442589902755), UINT64_C(7740639782147942024)},
        int128::uint128_t {UINT64_C(12855504354071922204), UINT64_C(6192511825718353619)},
        int128::uint128_t {UINT64_C(10284403483257537763), UINT64_C(8643358275316593218)},
        int128::uint128_t {UINT64_C(16455045573212060421), UINT64_C(10140024425764638826)},
        int128::uint128_t {UINT64_C(13164036458569648337), UINT64_C(4422670725869800738)},
        int128::uint128_t {UINT64_C(10531229166855718669), UINT64_C(14606183024921571560)},
        int128::uint128_t {UINT64_C(16849966666969149871), UINT64_C(12301846395648783526)},
        int128::uint128_t {UINT64_C(13479973333575319897), UINT64_C(6152128301777116498)},
        int128::uint128_t {UINT64_C(10783978666860255917), UINT64_C(15989749085647424168)},
        int128::uint128_t {UINT64_C(17254365866976409468), UINT64_C(10826203278068237376)},
        int128::uint128_t {UINT64_C(13803492693581127574), UINT64_C(16039660251938410547)},
        int128::uint128_t {UINT64_C(11042794154864902059), UINT64_C(16521077016292638761)},
        int128::uint128_t {UINT64_C(17668470647783843295), UINT64_C(15365676781842491048)},
        int128::uint128_t {UINT64_C(14134776518227074636), UINT64_C(12292541425473992838)},
        int128::uint128_t {UINT64_C(11307821214581659709), UINT64_C(6144684325637283947)},
        int128::uint128_t {UINT64_C(18092513943330655534), UINT64_C(17210192550503474962)},
        int128::uint128_t {UINT64_C(14474011154664524427), UINT64_C(17457502855144690293)},
        int128::uint128_t {UINT64_C(11579208923731619542), UINT64_C(6587304654631931588)},
        int128::uint128_t {UINT64_C(9263367138985295633), UINT64_C(16337890167931276240)},
        int128::uint128_t {UINT64_C(14821387422376473014), UINT64_C(4004531380238580045)},
        int128::uint128_t {UINT64_C(11857109937901178411), UINT64_C(6892973918932774359)},
        int128::uint128_t {UINT64_C(9485687950320942729), UINT64_C(1825030320404309164)},
        int128::uint128_t {UINT64_C(15177100720513508366), UINT64_C(10298746142130715309)},
        int128::uint128_t {UINT64_C(12141680576410806693), UINT64_C(4549648098962661924)},
        int128::uint128_t {UINT64_C(9713344461128645354), UINT64_C(11018416108653950185)},
        int128::uint128_t {UINT64_C(15541351137805832567), UINT64_C(6561419329620589327)},
        int128::uint128_t {UINT64_C(12433080910244666053), UINT64_C(16317181907922202431)},
        int128::uint128_t {UINT64_C(9946464728195732843), UINT64_C(1985699082112030975)},
        int128::uint128_t {UINT64_C(15914343565113172548), UINT64_C(17934513790346890853)},
        int128::uint128_t {UINT64_C(12731474852090538039), UINT64_C(3279564588051781713)},
        int128::uint128_t {UINT64_C(10185179881672430431), UINT64_C(6313000485183335694)}
    }};
    static constexpr std::array<int, 38> q126_shift = {{114, 118, 121, 124, 128, 131, 134, 138, 141, 144, 148, 151, 154, 158, 161, 164, 168, 171, 174, 178, 181, 184, 188, 191, 194, 197, 201, 204, 207, 211, 214, 217, 221, 224, 227, 231, 234, 237}};

    // 10^37, to turn a Q126 raw value n into the decimal (n * 10^37 >> 126) * 10^-37
    static constexpr int128::uint128_t pow10_37 {UINT64_C(542101086242752217), UINT64_C(68739955140067328)};
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b> constexpr std::array<std::uint64_t, 28> fixed_point_table_imp<b>::q62_recip;
template <bool b> constexpr std::array<int, 28> fixed_point_table_imp<b>::q62_shift;
template <bool b> constexpr std::array<int128::uint128_t, 38> fixed_point_table_imp<b>::q126_recip;
template <bool b> constexpr std::array<int, 38> fixed_point_table_imp<b>::q126_shift;
template <bool b> constexpr int128::uint128_t fixed_point_table_imp<b>::pow10_37;

#endif

using fixed_point_table = fixed_point_table_imp<true>;

// Qm fixed point: the raw integer n stands for n / 2^m. Q62 raw values are std::int64_t
// (step 2^-62), Q126 raw values are int128::int128_t (step 2^-126); both hold |v| < 2.
template <typename Raw>
struct q_format;

template <>
struct q_format<std::int64_t>
{
    // Raw Q62 value of t = sig * 10^e10, zero once t is below the Q62 step; t must be below 1,
    // since e10 > -7 wraps idx past the table and gives zero
    static constexpr auto from_decimal(const std::uint64_t sig, const int e10) noexcept -> std::uint64_t
    {
        const auto idx {static_cast<std::size_t>(-e10 - 7)};
        if (sig == 0U || idx >= fixed_point_table::q62_recip.size())
        {
            return 0U;
        }
        return static_cast<std::uint64_t>((int128::uint128_t {sig} * fixed_point_table::q62_recip[idx]) >> fixed_point_table::q62_shift[idx]);
    }

    // (a * b) >> 62 with the sign of a
    static constexpr auto mul(const std::int64_t a, const std::uint64_t b) noexcept -> std::int64_t
    {
        const auto m {a < 0 ? UINT64_C(0) - static_cast<std::uint64_t>(a) : static_cast<std::uint64_t>(a)};
        const auto p {static_cast<std::int64_t>(static_cast<std::uint64_t>((int128::uint128_t {m} * b) >> 62))};
        return a < 0 ? -p : p;
    }

    // n / 2^62 as the decimal (n * 10^18 >> 62) * 10^-18
    template <typename T>
    static constexpr auto to_decimal(const std::int64_t n) noexcept -> T
    {
        const auto m {n < 0 ? UINT64_C(0) - static_cast<std::uint64_t>(n) : static_cast<std::uint64_t>(n)};
        const auto sig {static_cast<std::uint64_t>((int128::uint128_t {m} * UINT64_C(1000000000000000000)) >> 62)};
        return T {sig, -18, n < 0 ? construction_sign::negative : construction_sign::positive};
    }
};

template <>
struct q_format<int128::int128_t>
{
    // Raw Q126 value of t = sig * 10^e10, zero once t is below the Q126 step; t must be below 1,
    // since e10 > -34 wraps idx past the table and gives zero
    static constexpr auto from_decimal(const int128::uint128_t sig, const int e10) noexcept -> int128::uint128_t
    {
        const auto idx {static_cast<std::size_t>(-e10 - 34)};
        if (sig == 0U || idx >= fixed_point_table::q126_recip.size())
        {
            return 0U;
        }
        return static_cast<int128::uint128_t>(umul256(sig, fixed_point_table::q126_recip[idx]) >> fixed_point_table::q126_shift[idx]);
    }

    // (a * b) >> 126 with the sign of a
    static constexpr auto mul(const int128::int128_t a, const int128::uint128_t b) noexcept -> int128::int128_t
    {
        const int128::uint128_t m {a < 0 ? -a : a};
        const int128::int128_t p {static_cast<int128::uint128_t>(umul256(m, b) >> 126)};
        return a < 0 ? -p : p;
    }

    // n / 2^126 as the decimal (n * 10^37 >> 126) * 10^-37
    template <typename T>
    static constexpr auto to_decimal(const int128::int128_t n) noexcept -> T
    {
        const int128::uint128_t m {n < 0 ? -n : n};
        const auto sig {static_cast<int128::uint128_t>(umul256(m, fixed_point_table::pow10_37) >> 126)};
        return T {sig, -37, n < 0 ? construction_sign::negative : construction_sign::positive};
    }
};

} // namespace fixed_point_detail

// x^3 P(x^2) from raw fixed-point coefficients of P, highest power first: u = x^2 P(x^2) is
// formed in fixed point and rounded once to T, and the result is x * u
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T, typename Raw, std::size_t N>
constexpr auto fixed_point_odd_tail(T x, const std::array<Raw, N>& coeffs) noexcept -> T
{
    using format = fixed_point_detail::q_format<Raw>;

    const T t {x * x};
    int e10 {};
    const auto sig {frexp10(t, &e10)};
    const auto tq {format::from_decimal(sig, e10)};

    Raw r {coeffs[0]};
    for (std::size_t i {1}; i < N; ++i)
    {
        r = format::mul(r, tq) + coeffs[i];
    }

    return x * format::template to_decimal<T>(format::mul(r, tq));
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP
