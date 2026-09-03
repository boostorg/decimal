// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_LOG1P_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_LOG1P_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/impl/taylor_series_result.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

namespace log1p_detail {

template <bool b>
struct log1p_table_imp
{
private:
    using d32_coeffs_t  = std::array<decimal32_t,   6>;
    using d64_coeffs_t  = std::array<decimal64_t,  15>;
    using d128_coeffs_t = std::array<decimal128_t, 33>;

    using d32_fast_coeffs_t  = std::array<decimal_fast32_t,   6>;
    using d64_fast_coeffs_t  = std::array<decimal_fast64_t,  15>;
    using d128_fast_coeffs_t = std::array<decimal_fast128_t, 33>;

public:
    static constexpr d32_coeffs_t d32_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 13}], without the first term
         boost::decimal::decimal32_t { UINT64_C(6666666666666666667), -19 }, // * w^3
         boost::decimal::decimal32_t { UINT64_C(4000000000000000000), -19 }, // * w^5
         boost::decimal::decimal32_t { UINT64_C(2857142857142857143), -19 }, // * w^7
         boost::decimal::decimal32_t { UINT64_C(2222222222222222222), -19 }, // * w^9
         boost::decimal::decimal32_t { UINT64_C(1818181818181818182), -19 }, // * w^11
         boost::decimal::decimal32_t { UINT64_C(1538461538461538462), -19 }, // * w^13
    }};

    static constexpr d32_fast_coeffs_t d32_fast_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 13}], without the first term
         boost::decimal::decimal_fast32_t { UINT64_C(6666666666666666667), -19 }, // * w^3
         boost::decimal::decimal_fast32_t { UINT64_C(4000000000000000000), -19 }, // * w^5
         boost::decimal::decimal_fast32_t { UINT64_C(2857142857142857143), -19 }, // * w^7
         boost::decimal::decimal_fast32_t { UINT64_C(2222222222222222222), -19 }, // * w^9
         boost::decimal::decimal_fast32_t { UINT64_C(1818181818181818182), -19 }, // * w^11
         boost::decimal::decimal_fast32_t { UINT64_C(1538461538461538462), -19 }, // * w^13
    }};

    static constexpr d64_coeffs_t d64_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 31}], without the first term
         boost::decimal::decimal64_t { UINT64_C(6666666666666666667), -19 },     // * w^3
         boost::decimal::decimal64_t { UINT64_C(4000000000000000000), -19 },     // * w^5
         boost::decimal::decimal64_t { UINT64_C(2857142857142857143), -19 },     // * w^7
         boost::decimal::decimal64_t { UINT64_C(2222222222222222222), -19 },     // * w^9
         boost::decimal::decimal64_t { UINT64_C(1818181818181818182), -19 },     // * w^11
         boost::decimal::decimal64_t { UINT64_C(1538461538461538462), -19 },     // * w^13
         boost::decimal::decimal64_t { UINT64_C(1333333333333333333), -19 },     // * w^15
         boost::decimal::decimal64_t { UINT64_C(1176470588235294118), -19 },     // * w^17
         boost::decimal::decimal64_t { UINT64_C(1052631578947368421), -19 },     // * w^19
         boost::decimal::decimal64_t { UINT64_C(9523809523809523810), -19 - 1 }, // * w^21
         boost::decimal::decimal64_t { UINT64_C(8695652173913043478), -19 - 1 }, // * w^23
         boost::decimal::decimal64_t { UINT64_C(8000000000000000000), -19 - 1 }, // * w^25
         boost::decimal::decimal64_t { UINT64_C(7407407407407407407), -19 - 1 }, // * w^27
         boost::decimal::decimal64_t { UINT64_C(6896551724137931034), -19 - 1 }, // * w^29
         boost::decimal::decimal64_t { UINT64_C(6451612903225806452), -19 - 1 }, // * w^31
    }};

    static constexpr d64_fast_coeffs_t d64_fast_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 31}], without the first term
         boost::decimal::decimal_fast64_t { UINT64_C(6666666666666666667), -19 },     // * w^3
         boost::decimal::decimal_fast64_t { UINT64_C(4000000000000000000), -19 },     // * w^5
         boost::decimal::decimal_fast64_t { UINT64_C(2857142857142857143), -19 },     // * w^7
         boost::decimal::decimal_fast64_t { UINT64_C(2222222222222222222), -19 },     // * w^9
         boost::decimal::decimal_fast64_t { UINT64_C(1818181818181818182), -19 },     // * w^11
         boost::decimal::decimal_fast64_t { UINT64_C(1538461538461538462), -19 },     // * w^13
         boost::decimal::decimal_fast64_t { UINT64_C(1333333333333333333), -19 },     // * w^15
         boost::decimal::decimal_fast64_t { UINT64_C(1176470588235294118), -19 },     // * w^17
         boost::decimal::decimal_fast64_t { UINT64_C(1052631578947368421), -19 },     // * w^19
         boost::decimal::decimal_fast64_t { UINT64_C(9523809523809523810), -19 - 1 }, // * w^21
         boost::decimal::decimal_fast64_t { UINT64_C(8695652173913043478), -19 - 1 }, // * w^23
         boost::decimal::decimal_fast64_t { UINT64_C(8000000000000000000), -19 - 1 }, // * w^25
         boost::decimal::decimal_fast64_t { UINT64_C(7407407407407407407), -19 - 1 }, // * w^27
         boost::decimal::decimal_fast64_t { UINT64_C(6896551724137931034), -19 - 1 }, // * w^29
         boost::decimal::decimal_fast64_t { UINT64_C(6451612903225806452), -19 - 1 }, // * w^31
    }};

    static constexpr d128_coeffs_t d128_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 67}], without the first term
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(361400724161834), UINT64_C(14966504185106442923) }, -34 }, // * w^3
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(216840434497100), UINT64_C(16358600140547686400) }, -34 }, // * w^5
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(154886024640786), UINT64_C(6414216079331332681) }, -34 },  // * w^7
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(120466908053944), UINT64_C(17286664110841848718) }, -34 }, // * w^9
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(98563833862318), UINT64_C(10789680804559775930) }, -34 },  // * w^11
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(83400167114269), UINT64_C(10548710224912852834) }, -34 },  // * w^13
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(72280144832366), UINT64_C(17750696095988929877) }, -34 },  // * w^15
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(63776598381500), UINT64_C(4811352982514025412) }, -34 },   // * w^17
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(57063272236079), UINT64_C(3334013506790993704) }, -34 },   // * w^19
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(516286748802621), UINT64_C(2933976190728223988) }, -35 },  // * w^21
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(471392248906741), UINT64_C(1074783124255373935) }, -35 },  // * w^23
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(433680868994201), UINT64_C(14270456207385821184) }, -35 }, // * w^25
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(401556360179816), UINT64_C(8430896172914024751) }, -35 },  // * w^27
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(373862818098449), UINT64_C(14846495913085646071) }, -35 }, // * w^29
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(349742636285646), UINT64_C(10913376164868902516) }, -35 }, // * w^31
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(328546112874395), UINT64_C(5221029225683333741) }, -35 },  // * w^33
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(309772049281572), UINT64_C(12828432158662665362) }, -35 }, // * w^35
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(293027614185271), UINT64_C(8645078838843957469) }, -35 },  // * w^37
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(278000557047565), UINT64_C(4417793960193590088) }, -35 },  // * w^39
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(264439554264757), UINT64_C(3302450641466607566) }, -35 },  // * w^41
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(252140040112908), UINT64_C(145889948468931370) }, -35 },   // * w^43
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(240933816107889), UINT64_C(16126584147974145820) }, -35 }, // * w^45
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(230681313294788), UINT64_C(3273345114337031103) }, -35 },  // * w^47
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(221265749486837), UINT64_C(11798414981003268347) }, -35 }, // * w^49
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(212588661271667), UINT64_C(9888928583810234167) }, -35 },  // * w^51
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(204566447638774), UINT64_C(7775502592561777065) }, -35 },  // * w^53
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(197127667724637), UINT64_C(3132617535410000244) }, -35 },  // * w^55
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(190210907453597), UINT64_C(4964463664733461809) }, -35 },  // * w^57
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(183763080082288), UINT64_C(16364473891814588694) }, -35 }, // * w^59
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(177738061063197), UINT64_C(8267792750398720369) }, -35 },  // * w^61
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(172095582934207), UINT64_C(977992063576074663) }, -35 },   // * w^63
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(166800334228539), UINT64_C(2650676376116154053) }, -35 },  // * w^65
         ::boost::decimal::decimal128_t { boost::int128::uint128_t { UINT64_C(161821219773955), UINT64_C(16337778628851158123) }, -35 }, // * w^67
    }};

    static constexpr d128_fast_coeffs_t d128_fast_coeffs =
    {{
         // Series[2*ArcTanh[w], {w, 0, 67}], without the first term
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(361400724161834), UINT64_C(14966504185106442923) }, -34 }, // * w^3
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(216840434497100), UINT64_C(16358600140547686400) }, -34 }, // * w^5
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(154886024640786), UINT64_C(6414216079331332681) }, -34 },  // * w^7
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(120466908053944), UINT64_C(17286664110841848718) }, -34 }, // * w^9
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(98563833862318), UINT64_C(10789680804559775930) }, -34 },  // * w^11
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(83400167114269), UINT64_C(10548710224912852834) }, -34 },  // * w^13
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(72280144832366), UINT64_C(17750696095988929877) }, -34 },  // * w^15
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(63776598381500), UINT64_C(4811352982514025412) }, -34 },   // * w^17
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(57063272236079), UINT64_C(3334013506790993704) }, -34 },   // * w^19
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(516286748802621), UINT64_C(2933976190728223988) }, -35 },  // * w^21
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(471392248906741), UINT64_C(1074783124255373935) }, -35 },  // * w^23
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(433680868994201), UINT64_C(14270456207385821184) }, -35 }, // * w^25
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(401556360179816), UINT64_C(8430896172914024751) }, -35 },  // * w^27
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(373862818098449), UINT64_C(14846495913085646071) }, -35 }, // * w^29
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(349742636285646), UINT64_C(10913376164868902516) }, -35 }, // * w^31
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(328546112874395), UINT64_C(5221029225683333741) }, -35 },  // * w^33
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(309772049281572), UINT64_C(12828432158662665362) }, -35 }, // * w^35
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(293027614185271), UINT64_C(8645078838843957469) }, -35 },  // * w^37
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(278000557047565), UINT64_C(4417793960193590088) }, -35 },  // * w^39
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(264439554264757), UINT64_C(3302450641466607566) }, -35 },  // * w^41
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(252140040112908), UINT64_C(145889948468931370) }, -35 },   // * w^43
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(240933816107889), UINT64_C(16126584147974145820) }, -35 }, // * w^45
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(230681313294788), UINT64_C(3273345114337031103) }, -35 },  // * w^47
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(221265749486837), UINT64_C(11798414981003268347) }, -35 }, // * w^49
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(212588661271667), UINT64_C(9888928583810234167) }, -35 },  // * w^51
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(204566447638774), UINT64_C(7775502592561777065) }, -35 },  // * w^53
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(197127667724637), UINT64_C(3132617535410000244) }, -35 },  // * w^55
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(190210907453597), UINT64_C(4964463664733461809) }, -35 },  // * w^57
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(183763080082288), UINT64_C(16364473891814588694) }, -35 }, // * w^59
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(177738061063197), UINT64_C(8267792750398720369) }, -35 },  // * w^61
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(172095582934207), UINT64_C(977992063576074663) }, -35 },   // * w^63
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(166800334228539), UINT64_C(2650676376116154053) }, -35 },  // * w^65
         ::boost::decimal::decimal_fast128_t { boost::int128::uint128_t { UINT64_C(161821219773955), UINT64_C(16337778628851158123) }, -35 }, // * w^67
    }};
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b>
constexpr typename log1p_table_imp<b>::d32_coeffs_t log1p_table_imp<b>::d32_coeffs;

template <bool b>
constexpr typename log1p_table_imp<b>::d64_coeffs_t log1p_table_imp<b>::d64_coeffs;

template <bool b>
constexpr typename log1p_table_imp<b>::d128_coeffs_t log1p_table_imp<b>::d128_coeffs;

template <bool b>
constexpr typename log1p_table_imp<b>::d32_fast_coeffs_t log1p_table_imp<b>::d32_fast_coeffs;

template <bool b>
constexpr typename log1p_table_imp<b>::d64_fast_coeffs_t log1p_table_imp<b>::d64_fast_coeffs;

template <bool b>
constexpr typename log1p_table_imp<b>::d128_fast_coeffs_t log1p_table_imp<b>::d128_fast_coeffs;

#endif

} //namespace log1p_detail

using log1p_table = log1p_detail::log1p_table_imp<true>;

// 2*atanh(w) = 2*w + (2/3)*w^3 + (2/5)*w^5 + ...
// The first coefficient is exactly 2 for every type, thus the tables leave it out and
// the caller adds that term itself. This gives the rest of the series, divided by w^3.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto log1p_series_tail(T z2) noexcept;

template <>
constexpr auto log1p_series_tail<decimal32_t>(decimal32_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d32_coeffs);
}

template <>
constexpr auto log1p_series_tail<decimal_fast32_t>(decimal_fast32_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d32_fast_coeffs);
}

template <>
constexpr auto log1p_series_tail<decimal64_t>(decimal64_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d64_coeffs);
}

template <>
constexpr auto log1p_series_tail<decimal_fast64_t>(decimal_fast64_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d64_fast_coeffs);
}

template <>
constexpr auto log1p_series_tail<decimal128_t>(decimal128_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d128_coeffs);
}

template <>
constexpr auto log1p_series_tail<decimal_fast128_t>(decimal_fast128_t z2) noexcept
{
    return taylor_series_result(z2, log1p_table::d128_fast_coeffs);
}

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_IMPL_LOG1P_IMPL_HPP
