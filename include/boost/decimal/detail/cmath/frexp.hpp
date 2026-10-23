// Copyright 2023 - 2026 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/cmath/decompose.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/add_impl.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <type_traits>
#include <limits>
#endif

namespace boost {
namespace decimal {

namespace detail {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127 4324)
#endif

template <bool b>
struct frexp_table_imp
{
    // 5^(+-d * 2^(4 * j)), d = 1..2^4-1, as M * 2^E with 2^255 <= M < 2^256, M rounded down.
    static constexpr int radix_bits { 4 };
    // 15 entries for each of the three lower digits and three for 16^3, so that |k5| <= 16383.
    static constexpr std::size_t size { 48 };
    // Rows of words and not u256, because Clang 6 to 8 do not evaluate the u256 constructor
    // in a constexpr member that comes from a precompiled header.
    static constexpr std::uint64_t pos5_sig[size][4] =
    {
        {UINT64_C(0xA000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xC800000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xFA00000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x9C40000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xC350000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xF424000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x9896800000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xBEBC200000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xEE6B280000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x9502F90000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xBA43B74000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xE8D4A51000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x9184E72A00000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xB5E620F480000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xE35FA931A0000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x8E1BC9BF04000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0x9DC5ADA82B70B59D), UINT64_C(0xF020000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xAF298D050E4395D6), UINT64_C(0x9670B12B7F410000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xC2781F49FFCFA6D5), UINT64_C(0x3CBF6B71C76B25FB), UINT64_C(0x50F8080000000000), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xD7E77A8F87DAF7FB), UINT64_C(0xDC33745EC97BE906), UINT64_C(0x3298E889D933B040), UINT64_C(0x0000000000000000)},
        {UINT64_C(0xEFB3AB16C59B14A2), UINT64_C(0xC5CFE94EF3EA101E), UINT64_C(0x388DA035C8F16477), UINT64_C(0xC22F1D0200000000)},
        {UINT64_C(0x850FADC09923329E), UINT64_C(0x03E2CF6BC604DDB0), UINT64_C(0x74A7EF0198791097), UINT64_C(0x51775F71E92BF2F2)},
        {UINT64_C(0x93BA47C980E98CDF), UINT64_C(0xC66F336C36B10137), UINT64_C(0x0234F3FD7B08DD39), UINT64_C(0x0BC3C54E3F40F7E6)},
        {UINT64_C(0xA402B9C5A8D3A6E7), UINT64_C(0x5F16206C9C6209A6), UINT64_C(0x39CAEF6ED62F905B), UINT64_C(0x618BA0B12F00D2F8)},
        {UINT64_C(0xB616A12B7FE617AA), UINT64_C(0x577B986B314D6009), UINT64_C(0x2381CF8591999D63), UINT64_C(0x95D7DDC421413571)},
        {UINT64_C(0xCA28A291859BBF93), UINT64_C(0x7D7B8F7503CFDCFE), UINT64_C(0xD11F91FF10629770), UINT64_C(0x56291E600B4D00DB)},
        {UINT64_C(0xE070F78D3927556A), UINT64_C(0x85BBE253F47B1417), UINT64_C(0x0F118A2758E233B1), UINT64_C(0xCEB740079A8B3D33)},
        {UINT64_C(0xF92E0C3537826145), UINT64_C(0xA7709A56CCDF8A82), UINT64_C(0x866CABA98A7E2DAB), UINT64_C(0x3A52513BB780474C)},
        {UINT64_C(0x8A5296FFE33CC92F), UINT64_C(0x82BD6B70D99AAA6F), UINT64_C(0xBC10C5C5CDA97C8D), UINT64_C(0xD7924BFF833149FA)},
        {UINT64_C(0x9991A6F3D6BF1765), UINT64_C(0xACCA6DA1E0A8EF29), UINT64_C(0x036EE4519D59A838), UINT64_C(0xD7FDA93B53D19020)},
        {UINT64_C(0xAA7EEBFB9DF9DE8D), UINT64_C(0xDDBB901B98FEEAB7), UINT64_C(0x851E4CBF3DE2F98A), UINT64_C(0xAE780C7FEA81C788)},
        {UINT64_C(0xE319A0AEA60E91C6), UINT64_C(0xCC655C54BC5058F8), UINT64_C(0x9C6583981D134CBA), UINT64_C(0x422D38EA3584CDE4)},
        {UINT64_C(0x973F9CA8CD00A68C), UINT64_C(0x6C8D3FCA02CA6DE6), UINT64_C(0xB0D7BA426777344D), UINT64_C(0x7CED4BDE6B367155)},
        {UINT64_C(0xC976758681750C17), UINT64_C(0x650D3D28F18B50CE), UINT64_C(0x526B988275249B0F), UINT64_C(0xD6F4B6D27BD1C61C)},
        {UINT64_C(0x862C8C0EEB856ECB), UINT64_C(0x085BCCD5C05EE9F9), UINT64_C(0xECFF2E2C1EAE9DB9), UINT64_C(0xB8A89F2441E0083F)},
        {UINT64_C(0xB2B8353B3993A7E4), UINT64_C(0x4257AC3B4C1D7794), UINT64_C(0x7704BD1BB5A5802F), UINT64_C(0x0BADB504E5728CB4)},
        {UINT64_C(0xEE0DDD84924AB88C), UINT64_C(0x2D4070F33B21AB7B), UINT64_C(0xC20578FA3851488B), UINT64_C(0xBF34FF7963028CD9)},
        {UINT64_C(0x9E8B3B5DC53D5DE4), UINT64_C(0xA74D28CE329ACE52), UINT64_C(0x6A3197BBEBE3034F), UINT64_C(0x77154CE2BCBA1964)},
        {UINT64_C(0xD32E203241F4806F), UINT64_C(0x3F50C802040F4CCC), UINT64_C(0x03BAA2F38E35464F), UINT64_C(0xE701F7BC8D1A0383)},
        {UINT64_C(0x8CA554C020A1F0A6), UINT64_C(0x5DFED09922680A06), UINT64_C(0xAEF839A8A7F6A14C), UINT64_C(0x126D388625960D50)},
        {UINT64_C(0xBB570A9A9BD977CC), UINT64_C(0x4C808753BB22FEF8), UINT64_C(0x6FC5802CDE0B3272), UINT64_C(0x10E980A1C0CCFD83)},
        {UINT64_C(0xF9895D25D88B5A8A), UINT64_C(0xFDD08C4DA13655EC), UINT64_C(0xF02C90B784B4227A), UINT64_C(0x7C9361E0FAB6D091)},
        {UINT64_C(0xA630EF7D5699FE45), UINT64_C(0x50E3660235410F98), UINT64_C(0xFCA81F202C5D111A), UINT64_C(0x0C7A3CBE3D82A042)},
        {UINT64_C(0xDD5DC8A2BF27F3F7), UINT64_C(0x95AA118EC1D08317), UINT64_C(0x8909E424A112A3CD), UINT64_C(0x134CA67A679B84AE)},
        {UINT64_C(0x936E07737DC64F6D), UINT64_C(0x8C474BB609F40287), UINT64_C(0xD2FEA4FD957EA18E), UINT64_C(0x6F75529546EE8B59)},
        {UINT64_C(0xC46052028A20979A), UINT64_C(0xC94C153F804A4A92), UINT64_C(0x65761FB2444E2267), UINT64_C(0xDD5CF7C945F22A3F)},
        {UINT64_C(0x96A3A1D17FAF211A), UINT64_C(0x0C7C2892305F4E12), UINT64_C(0x072B211ACEB5055E), UINT64_C(0x412E0901A7BB6C7D)},
        {UINT64_C(0xE71BEB05F96B70E4), UINT64_C(0x6B4BBE113A53EF59), UINT64_C(0x490A041D597A77CA), UINT64_C(0xA6FE0558E756761F)},
    };
    static constexpr int pos5_exp[size] = { -253, -251, -249, -246, -244, -242, -239, -237, -235, -232, -230, -228, -225, -223, -221, -218, -181, -144, -107, -70, -33, 5, 42, 79, 116, 153, 190, 227, 265, 302, 339, 933, 1528, 2122, 2717, 3311, 3905, 4500, 5094, 5689, 6283, 6877, 7472, 8066, 8661, 9255, 18766, 28276 };
    static constexpr std::uint64_t neg5_sig[size][4] =
    {
        {UINT64_C(0xCCCCCCCCCCCCCCCC), UINT64_C(0xCCCCCCCCCCCCCCCC), UINT64_C(0xCCCCCCCCCCCCCCCC), UINT64_C(0xCCCCCCCCCCCCCCCC)},
        {UINT64_C(0xA3D70A3D70A3D70A), UINT64_C(0x3D70A3D70A3D70A3), UINT64_C(0xD70A3D70A3D70A3D), UINT64_C(0x70A3D70A3D70A3D7)},
        {UINT64_C(0x83126E978D4FDF3B), UINT64_C(0x645A1CAC083126E9), UINT64_C(0x78D4FDF3B645A1CA), UINT64_C(0xC083126E978D4FDF)},
        {UINT64_C(0xD1B71758E219652B), UINT64_C(0xD3C36113404EA4A8), UINT64_C(0xC154C985F06F6944), UINT64_C(0x67381D7DBF487FCB)},
        {UINT64_C(0xA7C5AC471B478423), UINT64_C(0x0FCF80DC33721D53), UINT64_C(0xCDDD6E04C0592103), UINT64_C(0x85C67DFE32A0663C)},
        {UINT64_C(0x8637BD05AF6C69B5), UINT64_C(0xA63F9A49C2C1B10F), UINT64_C(0xD7E45803CD141A69), UINT64_C(0x37D1FE64F54D1E96)},
        {UINT64_C(0xD6BF94D5E57A42BC), UINT64_C(0x3D32907604691B4C), UINT64_C(0x8CA08CD2E1B9C3DB), UINT64_C(0x8C8330A1887B6424)},
        {UINT64_C(0xABCC77118461CEFC), UINT64_C(0xFDC20D2B36BA7C3D), UINT64_C(0x3D4D3D758161697C), UINT64_C(0x7068F3B46D2F8350)},
        {UINT64_C(0x89705F4136B4A597), UINT64_C(0x31680A88F8953030), UINT64_C(0xFDD7645E011ABAC9), UINT64_C(0xF387295D242602A6)},
        {UINT64_C(0xDBE6FECEBDEDD5BE), UINT64_C(0xB573440E5A884D1B), UINT64_C(0x2FBF06FCCE912ADC), UINT64_C(0xB8D8422EA03CD10A)},
        {UINT64_C(0xAFEBFF0BCB24AAFE), UINT64_C(0xF78F69A51539D748), UINT64_C(0xF2FF38CA3EDA88B0), UINT64_C(0x93E034F219CA40D5)},
        {UINT64_C(0x8CBCCC096F5088CB), UINT64_C(0xF93F87B7442E45D3), UINT64_C(0xF598FA3B657BA08D), UINT64_C(0x4319C3F4E16E9A44)},
        {UINT64_C(0xE12E13424BB40E13), UINT64_C(0x2865A5F206B06FB9), UINT64_C(0x88F4C3923BF900E2), UINT64_C(0x04F606549BE42A06)},
        {UINT64_C(0xB424DC35095CD80F), UINT64_C(0x538484C19EF38C94), UINT64_C(0x6D909C74FCC733E8), UINT64_C(0x03F805107CB68805)},
        {UINT64_C(0x901D7CF73AB0ACD9), UINT64_C(0x0F9D37014BF60A10), UINT64_C(0x57A6E390CA38F653), UINT64_C(0x3660040D3092066A)},
        {UINT64_C(0xE69594BEC44DE15B), UINT64_C(0x4C2EBE687989A9B3), UINT64_C(0xBF716C1ADD27F085), UINT64_C(0x23CCD3484DB670AA)},
        {UINT64_C(0xCFB11EAD453994BA), UINT64_C(0x67DE18EDA5814AF2), UINT64_C(0x0B5B1AA028CCD99E), UINT64_C(0x59E338E387AD8E28)},
        {UINT64_C(0xBB127C53B17EC159), UINT64_C(0x5560C018580D5D52), UINT64_C(0x3A63263A538DF733), UINT64_C(0x2D7C5B2ADC630227)},
        {UINT64_C(0xA87FEA27A539E9A5), UINT64_C(0x3F2398D747B36224), UINT64_C(0x2A1FEE40D90AAB31), UINT64_C(0x0E128B5D938CFB3F)},
        {UINT64_C(0x97C560BA6B0919A5), UINT64_C(0xDCCD879FC967D41A), UINT64_C(0x021DA8C6F15375A1), UINT64_C(0x3AD57881E73ED978)},
        {UINT64_C(0x88B402F7FD75539B), UINT64_C(0x11DBCB0218EBB414), UINT64_C(0x690C0DB23E2755EE), UINT64_C(0xE3EEB1A5E1589918)},
        {UINT64_C(0xF64335BCF065D37D), UINT64_C(0x4D4617B5FF4A16D5), UINT64_C(0xAA09501D5954A559), UINT64_C(0xEE19BFA6947F8E02)},
        {UINT64_C(0xDDD0467C64BCE4A0), UINT64_C(0xAC7CB3F6D05DDBDE), UINT64_C(0xE26CA6063461FFFA), UINT64_C(0x4ED775FC49F27952)},
        {UINT64_C(0xC7CABA6E7C5382C8), UINT64_C(0xFE64A52EE96B8FC0), UINT64_C(0xCBEB481C23D5E711), UINT64_C(0x6A618672EDB9DB26)},
        {UINT64_C(0xB3F4E093DB73A093), UINT64_C(0x59ED216765690F56), UINT64_C(0x8FE5B452E6B166CD), UINT64_C(0xD0C5B868313A262B)},
        {UINT64_C(0xA21727DB38CB002F), UINT64_C(0xB8ADA00E5A506A7C), UINT64_C(0xF1218F2B86615F63), UINT64_C(0xF924C5A3EF71B587)},
        {UINT64_C(0x91FF83775423CC06), UINT64_C(0x7B6306A34627DDCF), UINT64_C(0x1C5A40917D0FA664), UINT64_C(0x2E63F619DE93A2C6)},
        {UINT64_C(0x8380DEA93DA4BC60), UINT64_C(0x4247CB9E59F71E6D), UINT64_C(0x78B7AB3AF34A60C2), UINT64_C(0xCA9B9C54D7214150)},
        {UINT64_C(0xECE53CEC4A314EBD), UINT64_C(0xA4F8BF5635246428), UINT64_C(0x4609AC5C7899CA36), UINT64_C(0x9BDBFC21260DD1AD)},
        {UINT64_C(0xD5605FCDCF32E1D6), UINT64_C(0xFB1E4A9A90880A64), UINT64_C(0xEB30854F603DA8FC), UINT64_C(0x6FFC474C95A032DB)},
        {UINT64_C(0xC0314325637A1939), UINT64_C(0xFA911155FEFB5308), UINT64_C(0xA23E2ED27766E8CC), UINT64_C(0x9B03537708B1648F)},
        {UINT64_C(0x9049EE32DB23D21C), UINT64_C(0x7132D332E3F204D4), UINT64_C(0xE7317D62209B6A93), UINT64_C(0xD4C94A9DA0693E0C)},
        {UINT64_C(0xD8A66D4A505DE96B), UINT64_C(0x5AE1B25946117390), UINT64_C(0x4D0525AF79E132C3), UINT64_C(0x39F87391A061C7DD)},
        {UINT64_C(0xA2A682A5DA57C0BD), UINT64_C(0x87A601586BD3F698), UINT64_C(0xF53E94D1B2357C32), UINT64_C(0xC0EAFF3755A2DDCD)},
        {UINT64_C(0xF4385D0975EDBABE), UINT64_C(0x1F4BF6653CD3B977), UINT64_C(0xDDEE7F83569C8B33), UINT64_C(0xCD5140F638D331B3)},
        {UINT64_C(0xB759449F52A711B2), UINT64_C(0x68E1EB75340122D4), UINT64_C(0x0FD924BE26AF7592), UINT64_C(0xFAD3D6BB39DFD932)},
        {UINT64_C(0x89A63BA4C497B50E), UINT64_C(0x6C83AD1260FF20F4), UINT64_C(0xC098E6ED0BFBD6F6), UINT64_C(0xB62593291C768919)},
        {UINT64_C(0xCEAE534F34362DE4), UINT64_C(0x492512D4F2EAD2CB), UINT64_C(0x8263CA5CBC774BD9), UINT64_C(0x71AAD59046C74249)},
        {UINT64_C(0x9B2A840F28A1638F), UINT64_C(0xE393A9C032FB0C34), UINT64_C(0x660BDFD108BA7989), UINT64_C(0xE752A5FB23FC74C4)},
        {UINT64_C(0xE8FB7DC2DEC0A404), UINT64_C(0x598EEC7D41754C09), UINT64_C(0x5AD05B84C7C4BE7D), UINT64_C(0x49CE2BE1D65F0A25)},
        {UINT64_C(0xAEE973911228ABCA), UINT64_C(0xE3187C34500D9AB3), UINT64_C(0xB7D1F78B317FAE11), UINT64_C(0xF73AEC8B34D883C2)},
        {UINT64_C(0x8350BF3C91575A87), UINT64_C(0xE79E236BF8BF47A8), UINT64_C(0xE8A94DB92CA5841F), UINT64_C(0xF81E8A916CA9106B)},
        {UINT64_C(0xC52BA8A6AEB15D92), UINT64_C(0x9E98CB984F0D3050), UINT64_C(0xA42303E570B87E7F), UINT64_C(0x1C0B623F2299CB95)},
        {UINT64_C(0x9406AF8F83FD6265), UINT64_C(0x4B4DE34E0EBC3E06), UINT64_C(0x45EFB05F20CF48B3), UINT64_C(0x982B64E953AC4E27)},
        {UINT64_C(0xDE42FF8D37CAD87F), UINT64_C(0x1463EF488D5226CB), UINT64_C(0xB171E37A76C65371), UINT64_C(0x825B397E11354A97)},
        {UINT64_C(0xA6DD04C8D2CE9FDE), UINT64_C(0x2DE38123A1C3CFFC), UINT64_C(0x20305D0244E091BA), UINT64_C(0x5E2D7403972F6F2B)},
        {UINT64_C(0xD986C20B686DA869), UINT64_C(0x5D1D4FD85B05F4C2), UINT64_C(0xEEF183849AEE88F7), UINT64_C(0x6E7AD89A55416D56)},
        {UINT64_C(0x8DC92F39A90025DE), UINT64_C(0xFEDAF4534AEFF206), UINT64_C(0x28666B8C81058FA9), UINT64_C(0x2C1F1232DFA98B6A)},
    };
    static constexpr int neg5_exp[size] = { -258, -260, -262, -265, -267, -269, -272, -274, -276, -279, -281, -283, -286, -288, -290, -293, -330, -367, -404, -441, -478, -516, -553, -590, -627, -664, -701, -738, -776, -813, -850, -1444, -2039, -2633, -3228, -3822, -4416, -5011, -5605, -6200, -6794, -7388, -7983, -8577, -9172, -9766, -19277, -28787 };
    // 5^k for 0 <= k <= 27, the powers that fit in 64 bits
    static constexpr std::uint64_t pow5[28] =
    {
        UINT64_C(1), UINT64_C(5), UINT64_C(25), UINT64_C(125), UINT64_C(625), UINT64_C(3125), UINT64_C(15625),
        UINT64_C(78125), UINT64_C(390625), UINT64_C(1953125), UINT64_C(9765625), UINT64_C(48828125),
        UINT64_C(244140625), UINT64_C(1220703125), UINT64_C(6103515625), UINT64_C(30517578125), UINT64_C(152587890625),
        UINT64_C(762939453125), UINT64_C(3814697265625), UINT64_C(19073486328125), UINT64_C(95367431640625),
        UINT64_C(476837158203125), UINT64_C(2384185791015625), UINT64_C(11920928955078125),
        UINT64_C(59604644775390625), UINT64_C(298023223876953125), UINT64_C(1490116119384765625),
        UINT64_C(7450580596923828125)
    };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)
template <bool b> constexpr std::uint64_t frexp_table_imp<b>::pos5_sig[frexp_table_imp<b>::size][4];
template <bool b> constexpr int frexp_table_imp<b>::pos5_exp[frexp_table_imp<b>::size];
template <bool b> constexpr std::uint64_t frexp_table_imp<b>::neg5_sig[frexp_table_imp<b>::size][4];
template <bool b> constexpr int frexp_table_imp<b>::neg5_exp[frexp_table_imp<b>::size];
template <bool b> constexpr std::uint64_t frexp_table_imp<b>::pow5[28];
#endif

using frexp_table = frexp_table_imp<true>;

// a * b shifted right by the width of b and rounded down. The result has the width of a.
constexpr auto frexp_mulhi(const std::uint64_t a, const std::uint64_t b) noexcept -> std::uint64_t
{
    return (int128::uint128_t { a } * b).high;
}

constexpr auto frexp_mulhi(const int128::uint128_t& a, const int128::uint128_t& b) noexcept -> int128::uint128_t
{
    return umul256_hi(a, b);
}

constexpr auto frexp_mulhi(const int128::uint128_t& a, const std::uint64_t b) noexcept -> int128::uint128_t
{
    return int128::uint128_t { a.high } * b + (int128::uint128_t { a.low } * b).high;
}

constexpr auto frexp_mulhi(const u256& a, const u256& b) noexcept -> u256
{
    return umul512_hi(a, b);
}

// The top bits of x in the width of the second argument. Cut means rounded down,
// so that the table entries stay below the exact values.
constexpr auto frexp_cut(const u256& x, std::uint64_t) noexcept -> std::uint64_t { return x.bytes[3]; }
constexpr auto frexp_cut(const u256& x, const int128::uint128_t&) noexcept -> int128::uint128_t { return int128::uint128_t { x.bytes[3], x.bytes[2] }; }
constexpr auto frexp_cut(const u256& x, const u256&) noexcept -> u256 { return x; }
constexpr auto frexp_cut(const int128::uint128_t& x, std::uint64_t) noexcept -> std::uint64_t { return x.high; }
constexpr auto frexp_cut(const std::uint64_t x, std::uint64_t) noexcept -> std::uint64_t { return x; }

// The table entries cut to the word width W, so that a program keeps only the widths it uses.
// The narrow pass cuts these entries again to its word of half the width.
template <typename W>
struct frexp_cut_sig
{
    W pos[frexp_table::size];
    W neg[frexp_table::size];
};

template <typename W>
constexpr auto frexp_make_cut_sig() noexcept -> frexp_cut_sig<W>
{
    frexp_cut_sig<W> t {};
    for (std::size_t i { 0 }; i < frexp_table::size; ++i)
    {
        const auto& ps = frexp_table::pos5_sig[i];
        const auto& ns = frexp_table::neg5_sig[i];
        t.pos[i] = frexp_cut(u256 { ps[0], ps[1], ps[2], ps[3] }, W {});
        t.neg[i] = frexp_cut(u256 { ns[0], ns[1], ns[2], ns[3] }, W {});
    }
    return t;
}

template <typename W>
struct frexp_cut_table
{
    static constexpr frexp_cut_sig<W> sig { frexp_make_cut_sig<W>() };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)
template <typename W> constexpr frexp_cut_sig<W> frexp_cut_table<W>::sig;
#endif

// The significand s of bit length bits, shifted to the top of the word.
template <typename S> constexpr auto frexp_load(const S s, const int bits, std::uint64_t) noexcept -> std::uint64_t { return static_cast<std::uint64_t>(s) << (64 - bits); }
template <typename S> constexpr auto frexp_load(const S s, const int bits, const int128::uint128_t&) noexcept -> int128::uint128_t { return static_cast<int128::uint128_t>(s) << (128 - bits); }
template <typename S> constexpr auto frexp_load(const S s, const int bits, const u256&) noexcept -> u256 { return u256 { static_cast<int128::uint128_t>(s) << (128 - bits), int128::uint128_t { 0U } }; }

constexpr auto frexp_top_bit(const std::uint64_t x) noexcept -> bool { return (x >> 63U) != 0U; }
constexpr auto frexp_top_bit(const int128::uint128_t& x) noexcept -> bool { return (x.high >> 63U) != 0U; }
constexpr auto frexp_top_bit(const u256& x) noexcept -> bool { return (x.bytes[3] >> 63U) != 0U; }

// decimal32 and decimal64 split M into p + 1 digits (val) for fenv_round and a sticky bit. decimal128 keeps
// p digits (top), and val is their parity * 10 + frexp_rest, so that fenv_round does not divide them by 10.
template <typename D>
struct frexp_digits { D top; std::uint64_t val; bool sticky; };

// The digit for the bits frac below the digits: 0 (zero), 1 (below one half), 5 (one half) or 6.
template <typename W>
constexpr auto frexp_rest(const W& frac, const W& half) noexcept -> unsigned
{
    return static_cast<unsigned>(frac != W { 0U }) + 4U * static_cast<unsigned>(frac >= half) + static_cast<unsigned>(frac > half);
}

// A sticky bit lifts the digit 0 to 1, and one half (5) to above it (6).
constexpr auto frexp_lift(const unsigned rest, const bool sticky) noexcept -> unsigned
{
    return rest + static_cast<unsigned>(sticky && (rest == 0U || rest == 5U));
}

// An upper bound of the bit length of 5^k, because 76085 / 2^15 is just above log2(5).
constexpr auto frexp_pow5_bits(const int k) noexcept -> int { return ((k * 76085) >> 15) + 1; }

constexpr auto frexp_split(const std::uint64_t M, const int sh) noexcept -> frexp_digits<std::uint64_t>
{
    return { 0U, M >> sh, (M << (64 - sh)) != 0U };
}

constexpr auto frexp_split(const int128::uint128_t& M, const int sh) noexcept -> frexp_digits<std::uint64_t>
{
    return { 0U, static_cast<std::uint64_t>(M >> sh), (M << (128 - sh)) != 0U };
}

constexpr auto frexp_split(const u256& M, const int sh) noexcept -> frexp_digits<int128::uint128_t>
{
    // sh > 128, so the digits and the bits below them are in the top 128 bits.
    const int128::uint128_t hi { M.bytes[3], M.bytes[2] };
    const int128::uint128_t frac { hi << (256 - sh) };
    const int128::uint128_t half { UINT64_C(1) << 63U, 0U };
    // The two lower words act as a sticky bit.
    const unsigned rest { frexp_lift(frexp_rest(frac, half), (M.bytes[1] | M.bytes[0]) != 0U) };
    const int128::uint128_t top { hi >> (sh - 128) };
    return { top, (top.low & 1U) * 10U + rest, false };
}

// Sets sig to the p digits after fenv_round, and returns true if they rounded up to 10^p.
constexpr auto frexp_finish(const frexp_digits<std::uint64_t>&, const std::uint64_t val, const int r, std::uint64_t& sig) noexcept -> bool
{
    sig = val;
    // fenv_round returns 2 if the digits rounded up to 10^p.
    return r != 1;
}

constexpr auto frexp_finish(const frexp_digits<int128::uint128_t>& dg, const std::uint64_t val, int, int128::uint128_t& sig) noexcept -> bool
{
    sig = dg.top + (val != (dg.top.low & 1U) ? 1U : 0U);
    return sig == int128::uint128_t { UINT64_C(0x1ED09BEAD87C0), UINT64_C(0x378D8E6400000000) }; // 10^34
}

// The bit length of 10^n with n = p + 1 (decimal32, decimal64) or p (decimal128): 10^8, 10^17, 10^34.
constexpr auto frexp_ten_n_bits(const int p) noexcept -> int { return p <= 7 ? 27 : p <= 16 ? 57 : 113; }

// 10^n * 2^(256 - frexp_ten_n_bits(p)). Cut to a word, it is 10^n at the top of the word.
constexpr auto frexp_ten_n_256(const int p) noexcept -> u256
{
    return p <= 7 ? u256 { UINT64_C(0xBEBC200000000000), 0U, 0U, 0U } :
           p <= 16 ? u256 { UINT64_C(0xB1A2BC2EC5000000), 0U, 0U, 0U } :
           u256 { UINT64_C(0xF684DF56C3E01BC6), UINT64_C(0xC732000000000000), 0U, 0U };
}

// Factors of 5 in s cancel 5^k5 for k5 < k_end, so that an exact result stays exact.
// s < 10^p has fewer than 1.5 * p of them, so for a smaller k5 no result is exact.
template <int p, typename S>
constexpr auto frexp_cancel5(S& s, int& k5, const int k_end) noexcept -> void
{
    if (k5 < k_end && k5 - k_end >= -(p + p / 2))
    {
        // s * inv5 wraps around to s / 5 if 5 divides s, else to a value above max5.
        constexpr S max5 { static_cast<S>(~S { 0U } / 5U) };
        constexpr S inv5 { static_cast<S>(max5 * 4U + 1U) };
        while (k5 < k_end && static_cast<S>(s * inv5) <= max5) { s = static_cast<S>(s * inv5); ++k5; }
    }
}

// M * 2^E times 5^k5: one table entry for each nonzero digit of |k5| in radix 2^rb.
// M stays at the top of its word, and it is below the exact value.
template <typename W>
BOOST_DECIMAL_FORCE_INLINE constexpr auto frexp_mul_pow5(W& M, int& E, const int k5) noexcept -> void
{
    using cut_table = frexp_cut_table<W>;
    constexpr int rb { frexp_table::radix_bits };
    constexpr unsigned digit_mask { (1U << rb) - 1U };
    const bool neg_k { k5 < 0 };
    auto k { static_cast<unsigned>(neg_k ? -k5 : k5) };
    for (int base { 0 }; k != 0U; k >>= rb, base += static_cast<int>(digit_mask))
    {
        const unsigned dgt { k & digit_mask };
        if (dgt != 0U)
        {
            const auto idx { static_cast<std::size_t>(base + static_cast<int>(dgt) - 1) };
            M = frexp_mulhi(M, neg_k ? cut_table::sig.neg[idx] : cut_table::sig.pos[idx]);
            E += (neg_k ? frexp_table::neg5_exp[idx] : frexp_table::pos5_exp[idx]) + 256;
            if (!frexp_top_bit(M)) { M = M << 1; --E; }
        }
    }
}

// The same in a word NW of half the width of W, with the entries of W cut again. Returns the number
// of products nm. Mn is then below the exact value by less than 6 * nm + 1 units of its last bit.
template <typename W, typename NW>
BOOST_DECIMAL_FORCE_INLINE constexpr auto frexp_mul_pow5_narrow(NW& Mn, int& En, const int k5) noexcept -> unsigned
{
    using cut_table = frexp_cut_table<W>;
    constexpr int rb { frexp_table::radix_bits };
    constexpr unsigned digit_mask { (1U << rb) - 1U };
    unsigned nm { 0U };
    const bool neg_k { k5 < 0 };
    auto k { static_cast<unsigned>(neg_k ? -k5 : k5) };

    // For k5 >= 0, 5^d of the lowest digit d is exact in the top 64 bits of its entry,
    // because 5^27 < 2^64. So a 64-bit factor is enough.
    static_assert((1 << rb) - 1 <= 27, "5^d of one digit must fit in 64 bits");
    int base { 0 };
    const unsigned d0 { k & digit_mask };
    if (!neg_k)
    {
        if (d0 != 0U)
        {
            Mn = frexp_mulhi(Mn, frexp_cut(cut_table::sig.pos[d0 - 1U], std::uint64_t {}));
            En += frexp_table::pos5_exp[d0 - 1U] + 256;
            if (!frexp_top_bit(Mn)) { Mn = Mn << 1; --En; }
            ++nm;
        }
        k >>= rb;
        base = static_cast<int>(digit_mask);
    }
    for (; k != 0U; k >>= rb, base += static_cast<int>(digit_mask))
    {
        const unsigned dgt { k & digit_mask };
        if (dgt != 0U)
        {
            const auto idx { static_cast<std::size_t>(base + static_cast<int>(dgt) - 1) };
            Mn = frexp_mulhi(Mn, frexp_cut(neg_k ? cut_table::sig.neg[idx] : cut_table::sig.pos[idx], NW {}));
            En += (neg_k ? frexp_table::neg5_exp[idx] : frexp_table::pos5_exp[idx]) + 256;
            if (!frexp_top_bit(Mn)) { Mn = Mn << 1; --En; }
            ++nm;
        }
    }
    return nm;
}

// Rounds the p digits top once, with the bits frac below them. If the products were not exact,
// frac is only known to be below or above one half, and it is not one half or 0.
template <typename T, typename NW>
constexpr auto frexp_round_narrow(const NW& top, const NW& frac, const bool exact, const bool neg) noexcept -> NW
{
    const NW half { NW { 1U } << (sizeof(NW) * 8U - 1U) };
    const unsigned rest { exact ? frexp_rest(frac, half) : frac < half ? 1U : 6U };
    const std::uint64_t parity { static_cast<std::uint64_t>(top) & 1U };
    std::uint64_t last { parity * 10U + rest };
    fenv_round<T>(last, neg, false);
    return top + (last != parity ? 1U : 0U);
}

template <typename T>
constexpr auto frexp_impl(const T v, int* expon) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    // v = m * 2^e with 1/2 <= m < 1. With k5 = q + n, v * 10^n = s * 5^k5 * 2^k5, and s * 5^k5 = M * 2^E with M
    // at the top of a word of w bits. M shifted right gives the n leading digits of m, and fenv_round rounds them once.
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(v) || isinf(v))
    {
        if (expon != nullptr) { *expon = 0; }
        return isnan(v) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::infinity();
    }
    #endif

    // decompose, not fpclassify, so that the digits of v are not counted
    const auto comp { decompose(v) };

    if (comp.sig == 0U)
    {
        // A zero keeps its sign.
        if (expon != nullptr) { *expon = 0; }
        return v;
    }

    constexpr int p { std::numeric_limits<T>::digits10 };
    using word = std::conditional_t<(p <= 7), std::uint64_t, std::conditional_t<(p <= 16), int128::uint128_t, u256>>;
    constexpr int w { p <= 7 ? 64 : p <= 16 ? 128 : 256 };

    // n = p + 1 digits (decimal32, decimal64) or p digits (decimal128) come from M.
    constexpr int n_extra { p <= 16 ? 1 : 0 };
    constexpr int c_bits { frexp_ten_n_bits(p) };
    constexpr word c_norm { frexp_cut(frexp_ten_n_256(p), word {}) };

    auto s { comp.sig };
    using sig_type = decltype(s);
    int q { static_cast<int>(comp.exp) };
    int k5 { q + p + n_extra };

    frexp_cancel5<p>(s, k5, 0);
    const int bits { static_cast<int>(sizeof(s) * 8U) - countl_zero(s) };

    // All finite paths end in one pack_in_range. Else GCC merges the results of the fast types
    // with a memory copy, and that copy stalls on the stores of the single members.
    sig_type res_sig {};
    int res_exp { -p };
    int e {};
    bool done { false };

    // decimal64 and decimal128 first try p digits in a word of half the width. After nm products its M is below
    // the exact value by less than 6 * nm + 1 units of its last bit. The wide pass runs only if that can matter.
    if (p > 7)
    {
        using nword = std::conditional_t<(p <= 16), std::uint64_t, int128::uint128_t>;
        constexpr int nw { p <= 16 ? 64 : 128 };
        // The bit length of 10^p (10^16, 10^34), and 10^p * 2^(nw - n_bits).
        constexpr int n_bits { p <= 16 ? 54 : 113 };
        constexpr nword n_norm { frexp_cut(p <= 16 ? u256 { UINT64_C(0x8E1BC9BF04000000), 0U, 0U, 0U } : frexp_ten_n_256(p), nword {}) };
        constexpr nword ten_p { n_norm >> (nw - n_bits) };

        const int k5n { k5 - n_extra };
        nword Mn { frexp_load(s, bits, nword {}) };
        int En { bits - nw };
        // The products are exact if s * 5^k5n fits with one spare bit: k5n < 56 needs at most two entries,
        // and their bit lengths add up to at most bits(5^k5n) + 1.
        const bool exact { k5n >= 0 && bits + frexp_pow5_bits(k5n) < nw };
        const unsigned nm { frexp_mul_pow5_narrow<word>(Mn, En, k5n) };

        const bool ge { Mn >= n_norm };
        const int sh { nw - n_bits + (ge ? 1 : 0) };
        const nword frac { Mn << (nw - sh) };
        const nword half { nword { 1U } << (nw - 1) };
        const nword margin { nword { 6U * nm + 1U } << (nw - sh) };

        // M is below the exact value, so the bits below the digits must be nonzero, below 2^nw - margin
        // (no carry), and not within margin below one half.
        if (exact || (frac != nword { 0U } && frac < nword { 0U } - margin && (frac > half || frac <= half - margin)))
        {
            const nword sig { frexp_round_narrow<T>(nword { Mn >> sh }, frac, exact, signbit(v)) };

            const bool one { sig == ten_p };
            res_sig = one ? sig_type { 5U } : static_cast<sig_type>(sig);
            res_exp = one ? -1 : -p;
            e = En + q + p + sh + (one ? 1 : 0);
            done = true;
        }
    }

    if (!done)
    {
        word M { frexp_load(s, bits, word {}) };
        int E { bits - w };
        frexp_mul_pow5(M, E, k5);

        // v = (M / c_norm) * 2^(E + q + p + n_extra + w - c_bits), and M / c_norm is in (1/2, 2).
        const bool ge { M >= c_norm };
        const int sh { w - c_bits + (ge ? 1 : 0) };
        e = E + q + p + n_extra + sh;

        // m * 10^n = M * 2^-sh
        const auto dg { frexp_split(M, sh) };
        auto val { dg.val };
        const int r { fenv_round<T>(val, signbit(v), dg.sticky) };
        auto sig { dg.top };

        // If m rounds up to 1, it becomes 0.5 with e + 1.
        const bool one { frexp_finish(dg, val, r, sig) };
        res_sig = one ? sig_type { 5U } : static_cast<sig_type>(sig);
        res_exp = one ? -1 : -p;
        e += one ? 1 : 0;
    }

    if (expon != nullptr) { *expon = e; }

    return detail::pack_in_range<T>(res_sig, res_exp, signbit(v));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto frexp(const T v, int* expon) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    // A wider evaluation type can give a fraction that rounds to 1 in T, so frexp computes in T.
    return detail::frexp_impl(v, expon);
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_FREXP_HPP
