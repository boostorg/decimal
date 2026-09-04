// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// fma must calculate x * y + z with one rounding (IEEE 754-2019 fusedMultiplyAdd).
// The expected values come from MPFR at 900 bits, at the 34 digits of decimal128_t.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>

using namespace boost::decimal;

struct fma_case
{
    const char* x;
    const char* y;
    const char* z;
    const char* expected;
};

static const fma_case fma_cases[] =
{
// the three worst triples of the report
    { "4.891128916954585116359252097589272e+03",
      "-7.570510793333322422709743010426965e-02",
      "3.707536042839128637930999007415480e+02",
      "4.701617100187612469012746705158118e-01" },
    { "9.773367626875150075486696879175602e-02",
      "-3.337907954067831540381441089089419e-01",
      "3.235029480287355880201961692407601e-02",
      "-2.723067369020513807361798191291154e-04" },
    { "-1.635908948849659232280107483909349e+02",
      "-3.176446739966658464358952445855542e-03",
      "-5.281416298488918666313435068678353e-01",
      "-8.503865103313556434996443805914455e-03" },
// random triples, exponents of 10^-3 to 10^3
    { "-1.065008300110031905957897280440974e-03",
      "8.679786286031742647482469037679398e-01",
      "2.459772220945470025638742347671538e-01",
      "2.450528176507664992227350252829746e-01" },
    { "2.204698774688623268682332486262565e+02",
      "4.391348326395192744337294837450587e-01",
      "6.616668522588143528118897915024711e+03",
      "6.713484525332487707332688687817646e+03" },
    { "-9.976911973353208647336163725677450e-01",
      "2.169266061245795773782627422211721e-03",
      "-7.439068330870974128742758471652387e+00",
      "-7.441232588524957322211015428677276e+00" },
    { "-1.888740352881831634403692216579063e+01",
      "-7.900118649737038306011084617881166e+02",
      "-4.999754949433110540456096531467187e+01",
      "1.487127533681834187300900128042427e+04" },
    { "8.171530766625406270093349670323739e+03",
      "2.593580490537856789189978389609014e-03",
      "-5.576232872708608952707412919282934e-01",
      "2.063589948687864924607130452970817e+01" },
    { "-7.750345597256329137563280171352258e-02",
      "-9.100287754281406319757676608288459e+02",
      "3.073392767787947858334881593235580e+02",
      "3.778696519089553701144226749142327e+02" },
    { "6.013317852722187956978891138409258e-03",
      "1.570765883557867700949175676275650e-01",
      "-3.229835276084885365519028711089201e-02",
      "-3.135380130784430692263921963834903e-02" },
    { "9.033523195572541015573502066498907e+02",
      "-2.046124559993565407347510599148056e+01",
      "4.920017356632420627073578236505352e-03",
      "-1.848370875371517575917133434459618e+04" },
    { "7.517577000791313867554357371910665e+00",
      "8.135491485969028894823460725489820e-03",
      "7.227824998636771650829718582341538e+00",
      "7.288984182321825972393996547928244e+00" },
    { "-1.864205891267590352981025441417601e-03",
      "-4.189531285180449361628988649292932e-03",
      "-7.773177529299234971239730042481897e+02",
      "-7.773177451197745936407001508457541e+02" },
    { "8.739132880157219494689235001232555e+03",
      "-8.892335805012632280923256547689715e+02",
      "-4.914910797873741926771937139941189e+00",
      "-7.771135336409319086085617356230487e+06" },
    { "-1.396679541697529713725416669038118e+00",
      "7.849913763434619235676244454514602e+01",
      "-6.249747111291060839473824917109365e-02",
      "-1.097006370459028573861129989848511e+02" },
// random triples, exponents of 10^-40 to 10^40
    { "5.356601629996288981095041947483554e-06",
      "-1.107743045106680743270351100745053e+10",
      "-8.592767617963135435622045638865114e-25",
      "-5.933738201035498737909347544062264e+04" },
    { "9.512477724053467805288945525844576e+40",
      "1.944834564552786098638350181423332e+16",
      "-4.211117084368772467577488160835992e-02",
      "1.850019547227760382125089235816492e+57" },
    { "-2.305296013399484023822372416525407e-19",
      "7.655711025294320470305879153973029e+13",
      "-4.447673864198572267900470012730579e+15",
      "-4.447673864198572267918118692836928e+15" },
    { "-1.264876497583257991687893341033101e+24",
      "-3.742990996474187486609778516229004e+34",
      "-4.353475031420709188973353405088790e-28",
      "4.734421342105939030493612314372404e+58" },
    { "-6.229034438331746090377954099392618e+21",
      "5.878909887578624764747669923672911e-38",
      "8.356417713218954469061109119733163e-23",
      "-3.661992379315855424422692169895862e-16" },
    { "-5.367736664884465261136500864916400e-01",
      "1.446078880223927545847047179606676e-40",
      "4.552505687031048811640585814577223e-06",
      "4.552505687031048811640585814577223e-06" },
    { "7.825676209954665354533009758092445e+13",
      "7.348099629599945735417377676212569e-32",
      "3.227421877809614677045532446929357e+31",
      "3.227421877809614677045532446929357e+31" },
    { "-8.765245137926423343038120163995087e+36",
      "4.738139478573062887311022770079424e-04",
      "5.258003222290126591065982346675321e-05",
      "-4.153095402737977818771867378570165e+33" },
// full cancellation: z = -(x * y)
    { "-9.821591768821046489724273154673145e+03",
      "-5.695644916035523607121140304299307e-02",
      "-5.594029922546193912029752695961763e+02",
      "-4.960198068695729938168958164989485e-32" },
    { "-7.837759609545990790735737501578442e-01",
      "7.360064652792556650772937249901802e+02",
      "5.768641745930463709291680549854855e+02",
      "-1.871140972921262543667584500152484e-32" },
    { "2.577536822883360960346560946174337e+03",
      "-9.859520508228492288082021687542496e-01",
      "2.541327716593260836677977625895304e+03",
      "3.352094880709892407560976878748480e-32" },
    { "3.830620546279893542174381366992733e+03",
      "-4.816325532469494111462555739418184e+00",
      "1.844951554225009267514228026912495e+04",
      "1.726002516435586831681980823943128e-30" },
    { "-7.465631783731004512816281351465586e+01",
      "1.753606462212387395313298453174733e+02",
      "1.309178014044888207234059511576549e+04",
      "4.589490383525146685334075905761462e-30" },
    { "7.192489496627508038418482216553319e+01",
      "-1.730711828145351266092104188177397e+00",
      "1.244812664562443172694491501586712e+02",
      "-1.039506880225391261094969881130643e-32" },
// the addend is far above the product, and far below it
    { "3.912440344680615300327548748074514e+02",
      "-5.989563145097616740613295984033099e-03",
      "-2.343380849589202987305667436763179e+60",
      "-2.343380849589202987305667436763179e+60" },
    { "3.912440344680615300327548748074514e+02",
      "-5.989563145097616740613295984033099e-03",
      "-2.343380849589202987305667436763179e-60",
      "-2.343380849589202987305667436763179e+00" },
    { "1.867292908763428414483198545613648e+02",
      "-3.513550471806098126695521539413333e-02",
      "-6.560827880585925249028256532216146e+60",
      "-6.560827880585925249028256532216146e+60" },
    { "1.867292908763428414483198545613648e+02",
      "-3.513550471806098126695521539413333e-02",
      "-6.560827880585925249028256532216146e-60",
      "-6.560827880585925249028256532216146e+00" },
    { "-3.956554842866603775455866308051866e+03",
      "5.873050509509370135440315988880576e-02",
      "-2.323704643579947319873039743783247e+62",
      "-2.323704643579947319873039743783247e+62" },
    { "-3.956554842866603775455866308051866e+03",
      "5.873050509509370135440315988880576e-02",
      "-2.323704643579947319873039743783247e-58",
      "-2.323704643579947319873039743783247e+02" },
    { "-7.387551825407836147090904328834076e+01",
      "-5.605670026533070680680402587928557e-03",
      "4.141217783714837959539871536610098e+59",
      "4.141217783714837959539871536610098e+59" },
    { "-7.387551825407836147090904328834076e+01",
      "-5.605670026533070680680402587928557e-03",
      "4.141217783714837959539871536610098e-61",
      "4.141217783714837959539871536610098e-01" },
};

template <typename T>
static T parse(const char* str)
{
    T value {};
    from_chars(str, str + std::strlen(str), value);
    return value;
}

// Every case of the table, through the public fma and through unchecked_fma.
template <typename T>
void test_against_mpfr(const char* type_name)
{
    for (const auto& item : fma_cases)
    {
        const T x {parse<T>(item.x)};
        const T y {parse<T>(item.y)};
        const T z {parse<T>(item.z)};
        const T expected {parse<T>(item.expected)};

        if (!BOOST_TEST_EQ(fma(x, y, z), expected))
        {
            std::cerr << "  type: " << type_name << "\n     x: " << item.x << "\n     y: " << item.y
                      << "\n     z: " << item.z << "\n   got: " << fma(x, y, z) << "\n   ref: " << item.expected
                      << std::endl;
        }

        BOOST_TEST_EQ(detail::unchecked_fma(x, y, z), expected);
    }
}

// z = -(x * y) leaves the rounding error of the product alone. Two roundings give 0
// here, and one rounding gives the tail. The wider type calculates the same tail.
template <typename T, typename Wider>
void test_cancellation_against_wider()
{
    const T values[] =
    {
        T {UINT64_C(2736028658208323), -17},
        T {UINT64_C(9581230000000001), -3},
        T {UINT64_C(1234567890123457), -8},
        T {UINT64_C(8404866370008119), 2}
    };

    for (const auto& x : values)
    {
        for (const auto& y : values)
        {
            const T product {x * y};
            const T tail {fma(x, y, -product)};
            const auto wide_tail {static_cast<Wider>(x) * static_cast<Wider>(y) - static_cast<Wider>(product)};

            BOOST_TEST_EQ(tail, static_cast<T>(wide_tail));
        }
    }
}

// The same property for the 128 bit types, with the values of the table. The tail is
// exact, thus the test needs no wider type.
template <typename T>
void test_cancellation_is_not_zero()
{
    for (const auto& item : fma_cases)
    {
        const T x {parse<T>(item.x)};
        const T y {parse<T>(item.y)};
        const T product {x * y};

        if (product != T {0, 0})
        {
            const T tail {fma(x, y, -product)};
            BOOST_TEST_EQ(tail + product, product + tail);
            BOOST_TEST(abs(tail) < abs(product));
        }
    }
}

#ifndef BOOST_DECIMAL_FAST_MATH
template <typename T>
void test_non_finite()
{
    const T one {1, 0};
    const T zero {0, 0};
    const T inf {std::numeric_limits<T>::infinity()};
    const T nan {std::numeric_limits<T>::quiet_NaN()};

    BOOST_TEST_EQ(fma(inf, one, one), inf);
    BOOST_TEST_EQ(fma(-inf, one, one), -inf);
    BOOST_TEST_EQ(fma(one, inf, -one), inf);
    BOOST_TEST(isnan(fma(inf, zero, one)));
    BOOST_TEST(isnan(fma(nan, one, one)));
    BOOST_TEST(isnan(fma(one, one, nan)));
    BOOST_TEST(isnan(fma(inf, one, -inf)));

    // A product which is too large for the type gives infinity, and one which is too
    // small gives zero.
    const T big {std::numeric_limits<T>::max()};
    BOOST_TEST_EQ(fma(big, big, zero), inf);
    BOOST_TEST_EQ(fma(-big, big, zero), -inf);

    const T small {std::numeric_limits<T>::denorm_min()};
    BOOST_TEST_EQ(fma(small, small, zero), zero);
}
#endif

template <typename T>
void test_zero_operands()
{
    const T zero {0, 0};
    const T one {1, 0};
    const T value {parse<T>("3.707536042839128637930999007415480e+02")};

    BOOST_TEST_EQ(fma(zero, one, value), value);
    BOOST_TEST_EQ(fma(one, zero, value), value);
    BOOST_TEST_EQ(fma(value, one, zero), value);
    BOOST_TEST_EQ(fma(zero, zero, zero), zero);
    BOOST_TEST_EQ(fma(value, -one, value), zero);
}

// Every rounding mode gets the same two sums: one whose exact value needs more than 34
// digits, and one which falls exactly half way between two results.
template <typename T>
void test_rounding_modes()
{
    const T x {parse<T>("4.891128916954585116359252097589272e+03")};
    const T y {parse<T>("-7.570510793333322422709743010426965e-02")};
    const T z {parse<T>("3.707536042839128637930999007415480e+02")};
    const T one {parse<T>("1e+00")};
    const T half_ulp {parse<T>("5e-34")};

    // x * y + z is 4.70161710018761246901274670515811798...e-01
    const T high {parse<T>("4.701617100187612469012746705158118e-01")};

    fesetround(rounding_mode::fe_dec_to_nearest);
    BOOST_TEST_EQ(fma(x, y, z), high);
    BOOST_TEST_EQ(fma(-x, y, -z), -high);
    BOOST_TEST_EQ(fma(one, one, half_ulp), one);

    // fesetround changes the mode only when the library can find a constant evaluation.
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    const T low {parse<T>("4.701617100187612469012746705158117e-01")};
    const T tie_up {parse<T>("1.000000000000000000000000000000001e+00")};

    fesetround(rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST_EQ(fma(x, y, z), high);
    BOOST_TEST_EQ(fma(-x, y, -z), -high);
    BOOST_TEST_EQ(fma(one, one, half_ulp), tie_up);

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(fma(x, y, z), low);
    BOOST_TEST_EQ(fma(-x, y, -z), -high);
    BOOST_TEST_EQ(fma(one, one, half_ulp), one);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(fma(x, y, z), high);
    BOOST_TEST_EQ(fma(-x, y, -z), -low);
    BOOST_TEST_EQ(fma(one, one, half_ulp), tie_up);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(fma(x, y, z), low);
    BOOST_TEST_EQ(fma(-x, y, -z), -low);
    BOOST_TEST_EQ(fma(one, one, half_ulp), one);

    fesetround(rounding_mode::fe_dec_to_nearest);

    #endif
}

// A sum which falls exactly half way between two results goes to the even last digit.
// MPFR cannot give these values, because 5e-34 is not a dyadic rational.
template <typename T>
void test_exact_ties()
{
    const T one {parse<T>("1e+00")};

    // 1 + 0.5 ulp. The last digit of 1.000...000 is 0, thus the tie stays there.
    BOOST_TEST_EQ(fma(one, one, parse<T>("5e-34")), one);

    // The same sum from a product which is not trivial.
    BOOST_TEST_EQ(fma(parse<T>("2.5e-01"), parse<T>("4e+00"), parse<T>("5e-34")), one);

    // 1 + 1 ulp + 0.5 ulp. The last digit is 1, thus the tie goes up to 2.
    BOOST_TEST_EQ(fma(parse<T>("1.000000000000000000000000000000001"), one, parse<T>("5e-34")),
                  parse<T>("1.000000000000000000000000000000002"));

    // 1 + 2 ulp + 0.5 ulp. The last digit is 2, thus the tie stays there.
    BOOST_TEST_EQ(fma(parse<T>("1.000000000000000000000000000000002"), one, parse<T>("5e-34")),
                  parse<T>("1.000000000000000000000000000000002"));

    // The same three, with the sign changed.
    BOOST_TEST_EQ(fma(-one, one, parse<T>("-5e-34")), -one);
    BOOST_TEST_EQ(fma(parse<T>("-1.000000000000000000000000000000001"), one, parse<T>("-5e-34")),
                  parse<T>("-1.000000000000000000000000000000002"));

    // Just under and just over the tie.
    BOOST_TEST_EQ(fma(one, one, parse<T>("4e-34")), one);
    BOOST_TEST_EQ(fma(one, one, parse<T>("6e-34")), parse<T>("1.000000000000000000000000000000001"));

    // The tie from a subtraction: 1 - 0.5 ulp of the lower binade rounds to 1.
    BOOST_TEST_EQ(fma(one, one, parse<T>("-5e-35")), one);
}

// A product with one is exact, and an addend of zero adds nothing. These two identities
// check the alignment over the whole range of the exponent, against the operators.
template <typename T>
void test_identities(const int exponent_limit, const int count)
{
    std::mt19937_64 rng {42};
    std::uniform_int_distribution<int> digit {0, 9};
    std::uniform_int_distribution<int> lead {1, 9};
    std::uniform_int_distribution<int> exponent {-exponent_limit, exponent_limit};
    std::uniform_int_distribution<int> sign {0, 1};

    const auto make_value = [&]()
    {
        typename T::significand_type significand {static_cast<unsigned>(lead(rng))};
        for (int i {1}; i < std::numeric_limits<T>::digits10; ++i)
        {
            significand = significand * 10U + static_cast<unsigned>(digit(rng));
        }

        const T value {significand, exponent(rng)};
        return sign(rng) ? -value : value;
    };

    const T one {1, 0};
    const T zero {0, 0};

    for (int i {}; i < count; ++i)
    {
        const T x {make_value()};
        const T y {make_value()};
        const T z {make_value()};

        if (!BOOST_TEST_EQ(fma(x, one, z), x + z))
        {
            std::cerr << "  fma(x, 1, z) != x + z\n     x: " << x << "\n     z: " << z << std::endl;
        }

        if (!BOOST_TEST_EQ(fma(x, y, zero), x * y))
        {
            std::cerr << "  fma(x, y, 0) != x * y\n     x: " << x << "\n     y: " << y << std::endl;
        }
    }
}

int main()
{
    test_against_mpfr<decimal128_t>("decimal128_t");
    test_against_mpfr<decimal_fast128_t>("decimal_fast128_t");

    test_cancellation_is_not_zero<decimal128_t>();
    test_cancellation_is_not_zero<decimal_fast128_t>();

    test_cancellation_against_wider<decimal32_t, decimal64_t>();
    test_cancellation_against_wider<decimal_fast32_t, decimal_fast64_t>();
    test_cancellation_against_wider<decimal64_t, decimal128_t>();
    test_cancellation_against_wider<decimal_fast64_t, decimal_fast128_t>();

    #ifndef BOOST_DECIMAL_FAST_MATH
    test_non_finite<decimal32_t>();
    test_non_finite<decimal64_t>();
    test_non_finite<decimal128_t>();
    test_non_finite<decimal_fast128_t>();
    #endif

    test_zero_operands<decimal32_t>();
    test_zero_operands<decimal64_t>();
    test_zero_operands<decimal128_t>();
    test_zero_operands<decimal_fast128_t>();

    test_exact_ties<decimal128_t>();
    test_exact_ties<decimal_fast128_t>();

    test_rounding_modes<decimal128_t>();
    test_rounding_modes<decimal_fast128_t>();

    // The first range covers the alignment of the two operands. The second range goes
    // to the limits of the exponent, and it covers the subnormal results and the overflows.
    test_identities<decimal128_t>(60, 2000);
    test_identities<decimal_fast128_t>(60, 2000);
    test_identities<decimal128_t>(6200, 400);
    test_identities<decimal_fast128_t>(6200, 400);

    return boost::report_errors();
}
