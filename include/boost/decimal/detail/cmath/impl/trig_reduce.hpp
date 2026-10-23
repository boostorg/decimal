// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/integer_search_trees.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/cmath/impl/trig_fixed_point.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

template <bool b>
struct trig_table_imp
{
    // 2/pi = 0.636619772 367581343 ...: 6480 digits in words of 9 digits, which cover the window of
    // the largest decimal128_t exponent.
    static constexpr std::uint32_t two_over_pi[720] =
    {
        UINT32_C(636619772), UINT32_C(367581343), UINT32_C( 75535053), UINT32_C(490057448), UINT32_C(137838582), UINT32_C(961825794), UINT32_C(990669376), UINT32_C(235587190),
        UINT32_C(536906140), UINT32_C(360455211), UINT32_C( 65012343), UINT32_C(824291370), UINT32_C(907031832), UINT32_C(147571647), UINT32_C(384458314), UINT32_C(611511869),
        UINT32_C(642926799), UINT32_C(356916959), UINT32_C(867749636), UINT32_C(310292310), UINT32_C(985587701), UINT32_C(230754869), UINT32_C(571584869), UINT32_C(590646773),
        UINT32_C(449560966), UINT32_C(894516047), UINT32_C(329520456), UINT32_C(890799022), UINT32_C(863761847), UINT32_C(560347610), UINT32_C(695824481), UINT32_C(957643747),
        UINT32_C(751376342), UINT32_C(114892399), UINT32_C(785773600), UINT32_C(994689390), UINT32_C(957838443), UINT32_C(593292387), UINT32_C(132299624), UINT32_C(667945851),
        UINT32_C(218797794), UINT32_C(608751526), UINT32_C(299146267), UINT32_C(856964155), UINT32_C(983496557), UINT32_C(394439935), UINT32_C(472396799), UINT32_C(849771502),
        UINT32_C(340684715), UINT32_C(433724470), UINT32_C( 75068642), UINT32_C(186190147), UINT32_C(952038957), UINT32_C(841459037), UINT32_C(335072237), UINT32_C(209977986),
        UINT32_C(541221308), UINT32_C(627102012), UINT32_C(881299111), UINT32_C(265588664), UINT32_C( 91786992), UINT32_C(478392663), UINT32_C(362424067), UINT32_C(212143992),
        UINT32_C(535647949), UINT32_C(995331146), UINT32_C(617741119), UINT32_C( 20280064), UINT32_C(962710257), UINT32_C(555398285), UINT32_C(243520488), UINT32_C(797504590),
        UINT32_C(725511058), UINT32_C(951562532), UINT32_C(272185831), UINT32_C(913927045), UINT32_C(249709256), UINT32_C(279843100), UINT32_C( 98001191), UINT32_C( 39428356),
        UINT32_C(227611187), UINT32_C(140526100), UINT32_C(840065270), UINT32_C(984083699), UINT32_C(246424962), UINT32_C(245824812), UINT32_C(585936356), UINT32_C(993836765),
        UINT32_C(740846301), UINT32_C(630224803), UINT32_C(486106427), UINT32_C(208868636), UINT32_C(563029898), UINT32_C(330890390), UINT32_C(985141599), UINT32_C(500621317),
        UINT32_C(563255927), UINT32_C( 89637433), UINT32_C( 19188293), UINT32_C(314876162), UINT32_C(799903630), UINT32_C(630831397), UINT32_C(388157435), UINT32_C(931234869),
        UINT32_C(370256146), UINT32_C(758046650), UINT32_C(182823773), UINT32_C(310525074), UINT32_C(600104490), UINT32_C(871884612), UINT32_C(845039801), UINT32_C(754671780),
        UINT32_C(150502243), UINT32_C(345268467), UINT32_C(810390325), UINT32_C(128997664), UINT32_C(933372580), UINT32_C(424494147), UINT32_C(514252454), UINT32_C(546768668),
        UINT32_C(568278987), UINT32_C(840517002), UINT32_C(313344212), UINT32_C(478434378), UINT32_C( 39358226), UINT32_C(874839818), UINT32_C(986041726), UINT32_C(495262070),
        UINT32_C(323357771), UINT32_C(919883998), UINT32_C( 21017550), UINT32_C(264517783), UINT32_C(533227384), UINT32_C(203141166), UINT32_C( 60564161), UINT32_C(957195402),
        UINT32_C(555264310), UINT32_C(478797229), UINT32_C(364155998), UINT32_C(314767562), UINT32_C(392374951), UINT32_C( 88247501), UINT32_C(728908757), UINT32_C(205465021),
        UINT32_C( 44955121), UINT32_C(550155524), UINT32_C(427256270), UINT32_C(617363313), UINT32_C(114107733), UINT32_C(707198224), UINT32_C(283161544), UINT32_C(241410955),
        UINT32_C(984980503), UINT32_C(982997105), UINT32_C(188094376), UINT32_C(382337204), UINT32_C(659318564), UINT32_C(742310849), UINT32_C(623017797), UINT32_C(828087159),
        UINT32_C( 79169637), UINT32_C(961309179), UINT32_C( 80866598), UINT32_C(414261272), UINT32_C(614176015), UINT32_C(362759498), UINT32_C(870766355), UINT32_C( 52763866),
        UINT32_C( 27857619), UINT32_C(107882750), UINT32_C(734627112), UINT32_C(419119181), UINT32_C(801413583), UINT32_C( 33207527), UINT32_C(354751751), UINT32_C( 64499259),
        UINT32_C(812239862), UINT32_C(320876334), UINT32_C(395004140), UINT32_C(508516172), UINT32_C(926321994), UINT32_C(878747511), UINT32_C( 37862653), UINT32_C(848841368),
        UINT32_C(177634219), UINT32_C(914015170), UINT32_C(954777174), UINT32_C(146477511), UINT32_C(317149437), UINT32_C(513738812), UINT32_C(920948583), UINT32_C(351694228),
        UINT32_C(474545367), UINT32_C(717840732), UINT32_C(729167856), UINT32_C(660035132), UINT32_C(317325413), UINT32_C(991163989), UINT32_C(834597161), UINT32_C( 69802439),
        UINT32_C(574756378), UINT32_C(353220134), UINT32_C(812215221), UINT32_C(892492863), UINT32_C(237727907), UINT32_C( 41291325), UINT32_C(256759238), UINT32_C(999289753),
        UINT32_C(340697427), UINT32_C(959390004), UINT32_C(158002735), UINT32_C(520159146), UINT32_C(894398432), UINT32_C( 96010956), UINT32_C( 43499819), UINT32_C(419151694),
        UINT32_C(273044559), UINT32_C(795613075), UINT32_C(989708333), UINT32_C(984459683), UINT32_C(315615107), UINT32_C(138972142), UINT32_C( 18273824), UINT32_C(334685917),
        UINT32_C(233826893), UINT32_C(308141941), UINT32_C(570224808), UINT32_C(347357296), UINT32_C(398248847), UINT32_C( 13273576), UINT32_C( 83883174), UINT32_C(283099861),
        UINT32_C(995234744), UINT32_C(265443874), UINT32_C(647868149), UINT32_C(898168411), UINT32_C(324877007), UINT32_C(384899339), UINT32_C(964644598), UINT32_C(266224151),
        UINT32_C(878704559), UINT32_C(725131984), UINT32_C(310433111), UINT32_C(960403132), UINT32_C(144009353), UINT32_C( 91951634), UINT32_C(160955046), UINT32_C(229781723),
        UINT32_C(704047640), UINT32_C(217351993), UINT32_C(556186196), UINT32_C(849931806), UINT32_C(428291412), UINT32_C( 20908840), UINT32_C(944070093), UINT32_C(252692719),
        UINT32_C( 37244201), UINT32_C(312620437), UINT32_C(495654558), UINT32_C(581223170), UINT32_C(428720334), UINT32_C(471819506), UINT32_C(898583921), UINT32_C(895909169),
        UINT32_C(792436803), UINT32_C(748503147), UINT32_C(673331583), UINT32_C(545135961), UINT32_C(743474666), UINT32_C(559026937), UINT32_C(805638014), UINT32_C(549308766),
        UINT32_C(972455522), UINT32_C(655322903), UINT32_C(692110389), UINT32_C(380242192), UINT32_C(851112148), UINT32_C(261351132), UINT32_C(128683950), UINT32_C(939866273),
        UINT32_C(963201307), UINT32_C(954026967), UINT32_C(165858734), UINT32_C( 33126467), UINT32_C(413257344), UINT32_C(642923980), UINT32_C(599412479), UINT32_C(278935033),
        UINT32_C(776839366), UINT32_C(623816609), UINT32_C(  2573577), UINT32_C(251457761), UINT32_C(535534246), UINT32_C( 35190865), UINT32_C(800682588), UINT32_C(270075098),
        UINT32_C(242366434), UINT32_C(867431431), UINT32_C(756904939), UINT32_C( 25326844), UINT32_C(531994623), UINT32_C(766387562), UINT32_C(879402754), UINT32_C(976920230),
        UINT32_C( 76790822), UINT32_C(760152873), UINT32_C(570248813), UINT32_C(549694145), UINT32_C( 27233416), UINT32_C(626069188), UINT32_C(435246887), UINT32_C(183747330),
        UINT32_C(259540749), UINT32_C(998994834), UINT32_C(212466393), UINT32_C(224405568), UINT32_C(578178406), UINT32_C(459538110), UINT32_C(810045644), UINT32_C(280994086),
        UINT32_C(958980415), UINT32_C(466945615), UINT32_C(491440398), UINT32_C(699572694), UINT32_C(247248284), UINT32_C(696191559), UINT32_C(747554622), UINT32_C(769231394),
        UINT32_C(  9222822), UINT32_C(857625455), UINT32_C(452809474), UINT32_C( 80429640), UINT32_C(229943691), UINT32_C(244628878), UINT32_C(720159129), UINT32_C(903812006),
        UINT32_C(678340884), UINT32_C(921385675), UINT32_C( 94601741), UINT32_C(870585826), UINT32_C(263887604), UINT32_C(492339068), UINT32_C(397238834), UINT32_C(365134586),
        UINT32_C(676767107), UINT32_C(755165733), UINT32_C(262266026), UINT32_C(792528656), UINT32_C(608403582), UINT32_C(846914495), UINT32_C(370428271), UINT32_C(380704044),
        UINT32_C(538032027), UINT32_C(979073689), UINT32_C(427958499), UINT32_C(522063103), UINT32_C(923813588), UINT32_C(323419002), UINT32_C(390145062), UINT32_C(596137577),
        UINT32_C(816823271), UINT32_C(545742732), UINT32_C(168001260), UINT32_C(382378973), UINT32_C(757010179), UINT32_C(402699657), UINT32_C(163459005), UINT32_C(769213285),
        UINT32_C(329827804), UINT32_C(653978271), UINT32_C( 15757696), UINT32_C(144362175), UINT32_C(334211316), UINT32_C(973688139), UINT32_C(793746460), UINT32_C(586529144),
        UINT32_C( 99106666), UINT32_C(419812562), UINT32_C(629374302), UINT32_C(120563633), UINT32_C(119523659), UINT32_C(146773739), UINT32_C(690950410), UINT32_C(539991319),
        UINT32_C(828072647), UINT32_C(857284932), UINT32_C(561903051), UINT32_C(589936331), UINT32_C(564696389), UINT32_C(913055159), UINT32_C(672679975), UINT32_C(794999086),
        UINT32_C( 79592749), UINT32_C( 66517840), UINT32_C(732215833), UINT32_C(310083694), UINT32_C(540274155), UINT32_C(569138729), UINT32_C(890398901), UINT32_C(132030674),
        UINT32_C(277503346), UINT32_C(388916792), UINT32_C(977189896), UINT32_C(246552732), UINT32_C(455833226), UINT32_C(977394067), UINT32_C(714389532), UINT32_C(949570649),
        UINT32_C(609738007), UINT32_C(991239761), UINT32_C(608758453), UINT32_C(933709445), UINT32_C(470579965), UINT32_C(530861666), UINT32_C(425369931), UINT32_C(745496740),
        UINT32_C(244904434), UINT32_C(452847994), UINT32_C(533851388), UINT32_C(397673597), UINT32_C(709718236), UINT32_C(625133359), UINT32_C(619215284), UINT32_C(700046448),
        UINT32_C(466688207), UINT32_C(650317214), UINT32_C(211716964), UINT32_C(537612464), UINT32_C(536449981), UINT32_C(273543707), UINT32_C(833961775), UINT32_C(387231396),
        UINT32_C(389593123), UINT32_C(542118818), UINT32_C( 61221596), UINT32_C(560395479), UINT32_C(536353461), UINT32_C(934660889), UINT32_C(867449634), UINT32_C(901605616),
        UINT32_C( 36471496), UINT32_C(848818092), UINT32_C(301338958), UINT32_C(901525976), UINT32_C(155367623), UINT32_C(473692463), UINT32_C(785290977), UINT32_C(356264500),
        UINT32_C(649572425), UINT32_C(132781295), UINT32_C(533568526), UINT32_C(138225526), UINT32_C( 47008140), UINT32_C(434983823), UINT32_C(280449501), UINT32_C(743907262),
        UINT32_C(136074962), UINT32_C(957736145), UINT32_C(359121552), UINT32_C(688401812), UINT32_C(676731807), UINT32_C(795183670), UINT32_C(695816711), UINT32_C(516974110),
        UINT32_C(469628984), UINT32_C(237566410), UINT32_C(929131517), UINT32_C(872774596), UINT32_C(515798859), UINT32_C(813730210), UINT32_C(894366637), UINT32_C(192289919),
        UINT32_C(943224507), UINT32_C(602932875), UINT32_C(378107177), UINT32_C(340182320), UINT32_C(780997026), UINT32_C(522481950), UINT32_C(646453746), UINT32_C(135968115),
        UINT32_C( 18083422), UINT32_C(137657639), UINT32_C(620519309), UINT32_C( 98186364), UINT32_C(725288931), UINT32_C(362046664), UINT32_C(626028393), UINT32_C(502297349),
        UINT32_C(181945248), UINT32_C(164486865), UINT32_C(523662424), UINT32_C(644662928), UINT32_C(   333224), UINT32_C(458424725), UINT32_C(121305034), UINT32_C(783806409),
        UINT32_C(852866455), UINT32_C(430645921), UINT32_C(887973083), UINT32_C(108526576), UINT32_C(480637984), UINT32_C( 44253132), UINT32_C(208303833), UINT32_C(394012203),
        UINT32_C(163823399), UINT32_C(319287469), UINT32_C(611593542), UINT32_C( 55329582), UINT32_C(808323055), UINT32_C(902017169), UINT32_C( 39390588), UINT32_C(284065707),
        UINT32_C(897538017), UINT32_C(236663458), UINT32_C(113441299), UINT32_C(734417418), UINT32_C(628950231), UINT32_C(664546529), UINT32_C(648183123), UINT32_C(987886265),
        UINT32_C(360886352), UINT32_C(218317725), UINT32_C(313112022), UINT32_C( 98452835), UINT32_C(560749684), UINT32_C(843697956), UINT32_C(416402086), UINT32_C(198723884),
        UINT32_C(548830160), UINT32_C(228438536), UINT32_C(265725429), UINT32_C(817596639), UINT32_C( 77743155), UINT32_C(683173702), UINT32_C(471132088), UINT32_C(948045945),
        UINT32_C(699700956), UINT32_C(994914852), UINT32_C(528087066), UINT32_C(944302658), UINT32_C(239309043), UINT32_C(829662640), UINT32_C(937514974), UINT32_C(516528438),
        UINT32_C(994358860), UINT32_C(285229564), UINT32_C(162905741), UINT32_C(656718822), UINT32_C(889061919), UINT32_C(215260510), UINT32_C(383164960), UINT32_C(101378721),
        UINT32_C(928810469), UINT32_C(369196004), UINT32_C( 81932249), UINT32_C(852135185), UINT32_C(898712762), UINT32_C(  7247321), UINT32_C(500615211), UINT32_C(518093733),
        UINT32_C(678200854), UINT32_C(275908365), UINT32_C(162245727), UINT32_C(151516834), UINT32_C(482297999), UINT32_C(703159027), UINT32_C(607396841), UINT32_C(296825885),
        UINT32_C(540764555), UINT32_C(259025608), UINT32_C(390422195), UINT32_C(831751405), UINT32_C(656165812), UINT32_C(206063358), UINT32_C(571293061), UINT32_C(624082413),
        UINT32_C(247566346), UINT32_C(281088345), UINT32_C(  1079665), UINT32_C(575006111), UINT32_C(549442432), UINT32_C(458227793), UINT32_C(684128963), UINT32_C(109090968),
        UINT32_C(660545693), UINT32_C(746797086), UINT32_C(536123762), UINT32_C(122992261), UINT32_C( 74037206), UINT32_C(635685476), UINT32_C(856572517), UINT32_C(485364246),
        UINT32_C(286148562), UINT32_C(481591390), UINT32_C(473706011), UINT32_C(912314425), UINT32_C( 67879843), UINT32_C(236736893), UINT32_C(905340190), UINT32_C(986876069),
        UINT32_C(801805784), UINT32_C(665531384), UINT32_C(832963469), UINT32_C(438040948), UINT32_C(521161777), UINT32_C(511763414), UINT32_C( 13781770), UINT32_C(533652250),
        UINT32_C(522983805), UINT32_C(532124091), UINT32_C(725877378), UINT32_C(673314070), UINT32_C(653129660), UINT32_C(608407176), UINT32_C(905775828), UINT32_C(724868680),
        UINT32_C(870259687), UINT32_C(857797586), UINT32_C(128888750), UINT32_C(633952978), UINT32_C( 47637605), UINT32_C(362017728), UINT32_C(559434514), UINT32_C(484332717),
        UINT32_C(575843377), UINT32_C(559207659), UINT32_C(149559089), UINT32_C(324114524), UINT32_C( 52594782), UINT32_C( 85048207), UINT32_C(311225397), UINT32_C(828474651),
        UINT32_C(113026395), UINT32_C(324021406), UINT32_C(209266639), UINT32_C(375763608), UINT32_C(872252578), UINT32_C(180848519), UINT32_C(158937885), UINT32_C(954965033),
        UINT32_C( 72895440), UINT32_C(944108439), UINT32_C(924766082), UINT32_C(275293889), UINT32_C(593432053), UINT32_C(464273514), UINT32_C(531547171), UINT32_C(447892946),
        UINT32_C(901442674), UINT32_C( 86742528), UINT32_C( 47795912), UINT32_C(293583367), UINT32_C(676266383), UINT32_C(354714117), UINT32_C(649674872), UINT32_C(869119500),
        UINT32_C(244157842), UINT32_C(592783429), UINT32_C(824802435), UINT32_C(684913665), UINT32_C(577495386), UINT32_C(198359728), UINT32_C(113924945), UINT32_C(733864478),
        UINT32_C(829297238), UINT32_C(183436293), UINT32_C(447514516), UINT32_C(252740066), UINT32_C( 42507030), UINT32_C(740486543), UINT32_C( 35478522), UINT32_C(980799688),
        UINT32_C(  4310670), UINT32_C(732378792), UINT32_C(599024907), UINT32_C(297391746), UINT32_C(852433648), UINT32_C(408780835), UINT32_C(979276497), UINT32_C(761950046),
        UINT32_C(842367376), UINT32_C(559631557), UINT32_C(823100738), UINT32_C(486476166), UINT32_C(123738175), UINT32_C(211235754), UINT32_C(512292950), UINT32_C(314461071),
        UINT32_C(188457329), UINT32_C(296787943), UINT32_C(122255052), UINT32_C( 72353754), UINT32_C(656242870), UINT32_C(147328545), UINT32_C( 51868489), UINT32_C(704377141),
        UINT32_C(604438528), UINT32_C(730510604), UINT32_C(804680902), UINT32_C(117171586), UINT32_C(223784328), UINT32_C(197536362), UINT32_C(763042768), UINT32_C( 15818584),
        UINT32_C(766560086), UINT32_C(269344071), UINT32_C(638527491), UINT32_C(567994537), UINT32_C(364347612), UINT32_C(802318654), UINT32_C(841251444), UINT32_C(942795527),
        UINT32_C( 56145701), UINT32_C( 16334839), UINT32_C(243259340), UINT32_C(761248527), UINT32_C(449889127), UINT32_C(242033804), UINT32_C(947607625), UINT32_C(865289437),
    };

    // pi/2 = 1.570796326 794896619 ...
    static constexpr std::uint32_t pio2[6] = { UINT32_C(1), UINT32_C(570796326), UINT32_C(794896619), UINT32_C(231321691), UINT32_C(639751442), UINT32_C(98584699) };

    // For |x| < 10^19 in base 2^64, high word first: 2/pi and ceil(10^(-9a) * 2^448) for a = 1 to 4,
    // each word i with the weight 2^(-64(i + 1)), and pi/2 with word 0 as the integer part.
    static constexpr std::uint64_t two_over_pi_bin[7] =
    {
        UINT64_C(0xA2F9836E4E441529), UINT64_C(0xFC2757D1F534DDC0), UINT64_C(0xDB6295993C439041), UINT64_C(0xFE5163ABDEBBC561),
        UINT64_C(0xB7246E3A424DD2E0), UINT64_C(0x06492EEA09D1921C), UINT64_C(0xFE1DEB1CB129A73E)
    };

    static constexpr std::uint64_t pow10_neg9_bin[4][7] =
    {
        {UINT64_C(0x000000044B82FA09), UINT64_C(0xB5A52CB98B405447), UINT64_C(0xC4A98187EEBB22F0), UINT64_C(0x08D5D64F9C394AE9),
         UINT64_C(0x213015356022EF32), UINT64_C(0x164179B6BF082CE3), UINT64_C(0xFD84BF5BB9D3E58A)},
        {UINT64_C(0x0000000000000012), UINT64_C(0x725DD1D243ABA0E7), UINT64_C(0x5FE645CC4873F9E6), UINT64_C(0x5AFE688C928E1F21),
         UINT64_C(0x95818AE77F3C36A0), UINT64_C(0x8CCE4E0A36628033), UINT64_C(0xA40BE73647459D42)},
        {UINT64_C(0x0000000000000000), UINT64_C(0x0000004F3A68DBC8), UINT64_C(0xF03F243BAF513267), UINT64_C(0xAA9A3EE524F8E028),
         UINT64_C(0x9064E3CFFA15AB8B), UINT64_C(0xB9CCC2933B76B4FA), UINT64_C(0x41402348EBC5909A)},
        {UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000154), UINT64_C(0x484932D2E725A5BB), UINT64_C(0xCA17A3ABA173D3D5),
         UINT64_C(0xFC130C23B7AA2DA1), UINT64_C(0x9B9A3CAB811D56FA), UINT64_C(0x9C85A535DF608EEE)},
    };

    static constexpr std::uint64_t pio2_bin[5] =
    {
        UINT64_C(0x0000000000000001), UINT64_C(0x921FB54442D18469), UINT64_C(0x898CC51701B839A2), UINT64_C(0x52049C1114CF98E8),
        UINT64_C(0x04177D4C76273644)
    };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b>
constexpr std::uint32_t trig_table_imp<b>::two_over_pi[720];

template <bool b>
constexpr std::uint32_t trig_table_imp<b>::pio2[6];

template <bool b>
constexpr std::uint64_t trig_table_imp<b>::two_over_pi_bin[7];

template <bool b>
constexpr std::uint64_t trig_table_imp<b>::pow10_neg9_bin[4][7];

template <bool b>
constexpr std::uint64_t trig_table_imp<b>::pio2_bin[5];

#endif

using trig_table = trig_table_imp<true>;

// Words of 2/pi in the product window, words of the fraction that go into r, and fixed-point words.
// The window keeps 9 * (window - 1 - frac) = 27, 45 and 81 digits for the leading zeros of the
// fraction: the worst cases have 9, 19 and 37.
// bin_frac: 64-bit fraction words for |x| < 10^19, where the fraction has at most 27, 58 and 117 leading zero bits.
template <typename T>
struct trig_traits;

template <>
struct trig_traits<decimal32_t> { static constexpr int window = 6; static constexpr int frac = 2; static constexpr int words = 1; static constexpr int bin_frac = 2; };

template <>
struct trig_traits<decimal_fast32_t> { static constexpr int window = 6; static constexpr int frac = 2; static constexpr int words = 1; static constexpr int bin_frac = 2; };

template <>
struct trig_traits<decimal64_t> { static constexpr int window = 9; static constexpr int frac = 3; static constexpr int words = 2; static constexpr int bin_frac = 3; };

template <>
struct trig_traits<decimal_fast64_t> { static constexpr int window = 9; static constexpr int frac = 3; static constexpr int words = 2; static constexpr int bin_frac = 3; };

template <>
struct trig_traits<decimal128_t> { static constexpr int window = 15; static constexpr int frac = 5; static constexpr int words = 3; static constexpr int bin_frac = 5; };

template <>
struct trig_traits<decimal_fast128_t> { static constexpr int window = 15; static constexpr int frac = 5; static constexpr int words = 3; static constexpr int bin_frac = 5; };

constexpr std::uint64_t word_base {UINT64_C(1000000000)};

// x = n*pi/2 + r (mod 2pi), with |r| <= pi/4.
template <int N>
struct trig_arg
{
    boost::int128::uint128_t sig; // |r| = sig * 10^-k, with sig of 38 digits
    int k;
    fx<N> rf;                     // |r| in fixed point, if fixed is set
    bool fixed;
    bool neg;                     // the sign of r, which includes the sign of x
    unsigned n;                   // the quadrant 0 to 3, which includes the sign of x
    bool zero;                    // x = 0: sig and k are not set
};

// |r| in fixed point: the binary reduction gives it, else it comes from sig and k.
template <int N>
constexpr auto fixed_r(const trig_arg<N>& r) noexcept -> fx<N>
{
    return r.fixed ? r.rf : fx_from<N>(r.sig, r.k);
}

// The significand in words of 9 digits, low word first; returns the count of words.
constexpr auto to_words(std::uint64_t m, std::uint64_t* out) noexcept -> int
{
    int n {};
    while (m != 0U)
    {
        out[n++] = m % word_base;
        m /= word_base;
    }
    return n;
}

constexpr auto to_words(std::uint32_t m, std::uint64_t* out) noexcept -> int
{
    return to_words(static_cast<std::uint64_t>(m), out);
}

constexpr auto to_words(const boost::int128::uint128_t& m, std::uint64_t* out) noexcept -> int
{
    constexpr std::uint64_t base2 {UINT64_C(1000000000000000000)};
    const auto hi {static_cast<std::uint64_t>(m / base2)};
    const auto lo {static_cast<std::uint64_t>(m % base2)};
    out[0] = lo % word_base;
    out[1] = lo / word_base;
    out[2] = hi % word_base;
    out[3] = hi / word_base;
    // Zero high words do not change the product.
    return 4;
}

// m * 10^shift in words of 9 digits, low word first, for 0 <= shift < 9; returns the count of words.
template <typename Significand>
constexpr auto scaled_words(const Significand m, const int shift, std::uint64_t* out) noexcept -> int
{
    int count {to_words(m, out)};
    const auto scale {pow10(static_cast<std::uint64_t>(shift))};
    std::uint64_t carry {};
    for (int i {}; i < count; ++i)
    {
        const std::uint64_t v {out[i] * scale + carry};
        out[i] = v % word_base;
        carry = v / word_base;
    }
    if (carry != 0U)
    {
        out[count++] = carry;
    }
    return count;
}

// The low Window words of m * 10^(9 * word_exp) * 2/pi, low word first. Word Window - 1 is the unit
// word, and 10^9 = 0 (mod 4), thus its low two bits are the quadrant and the higher words are not needed.
template <int Window>
constexpr auto times_two_over_pi(const std::uint64_t* m, const int count, const int word_exp, std::uint64_t* out) noexcept -> void
{
    // two_over_pi[i] has the weight 10^(-9(i + 1)), thus word u of the window is table word Window - 2 - u + word_exp.
    std::uint64_t w[static_cast<std::size_t>(Window)] {};
    for (int u {}; u < Window; ++u)
    {
        const int i {Window - 2 - u + word_exp};
        w[u] = (i >= 0 && i < 720) ? static_cast<std::uint64_t>(trig_table::two_over_pi[i]) : 0U;
    }

    std::uint64_t carry {};
    for (int col {}; col < Window; ++col)
    {
        std::uint64_t acc {carry};
        for (int i {}; i < count && i <= col; ++i)
        {
            acc += m[i] * w[col - i];
        }
        out[col] = acc % word_base;
        carry = acc / word_base;
    }
}

// The fraction f in [0, 1) of Words words, high word first, to 1 - f when f >= 1/2; returns whether it did.
template <int Words>
constexpr auto fold_half(std::uint64_t* f) noexcept -> bool
{
    if (f[0] < word_base / 2U)
    {
        return false;
    }
    for (int i {}; i < Words; ++i)
    {
        f[i] = word_base - 1U - f[i];
    }
    for (int i {Words - 1}; i >= 0; --i)
    {
        if (++f[i] < word_base)
        {
            break;
        }
        f[i] = 0U;
    }
    return true;
}

// The Frac words of the fraction from word lead on, high word first, times pi/2: 2 * Frac + 1 words,
// low word first.
template <int Frac, int Words>
constexpr auto times_half_pi(const std::uint64_t* f, const int lead, std::uint64_t* out) noexcept -> void
{
    std::uint64_t a[static_cast<std::size_t>(Frac)] {};
    std::uint64_t b[static_cast<std::size_t>(Frac + 1)] {};
    for (int u {}; u < Frac; ++u)
    {
        const int i {lead + Frac - 1 - u};
        a[u] = i < Words ? f[i] : 0U;
    }
    for (int v {}; v <= Frac; ++v)
    {
        b[v] = static_cast<std::uint64_t>(trig_table::pio2[Frac - v]);
    }

    std::uint64_t carry {};
    for (int col {}; col <= 2 * Frac; ++col)
    {
        std::uint64_t acc {carry};
        for (int u {col - Frac < 0 ? 0 : col - Frac}; u < Frac && u <= col; ++u)
        {
            acc += a[u] * b[col - u];
        }
        out[col] = acc % word_base;
        carry = acc / word_base;
    }
}

// The first 38 digits of the nonzero words r, low word first, as sig * 10^exp10 with word 0 at 10^0.
constexpr auto first_38_digits(const std::uint64_t* r, const int count, int& exp10) noexcept -> boost::int128::uint128_t
{
    int top {count - 1};
    while (r[top] == 0U)
    {
        --top;
    }
    boost::int128::uint128_t sig {r[top]};
    int digits {num_digits(r[top])};
    exp10 = 9 * top;
    for (int i {top - 1}; i >= 0; --i)
    {
        if (digits + 9 <= 38)
        {
            sig = sig * word_base + r[i];
            digits += 9;
            exp10 -= 9;
        }
        else
        {
            const int take {38 - digits};
            const auto low {pow10(static_cast<std::uint64_t>(9 - take))};
            sig = sig * (word_base / low) + r[i] / low;
            exp10 -= take;
            break;
        }
    }

    const int sig_digits {num_digits(sig)};
    if (sig_digits < 38)
    {
        sig *= pow10(static_cast<boost::int128::uint128_t>(38 - sig_digits));
        exp10 -= 38 - sig_digits;
    }
    return sig;
}

// Payne-Hanek reduction in words of 9 decimal digits for x = m * 10^e: multiply m by the window of
// 2/pi which starts at the exponent, then the integer part mod 4 is the quadrant.
template <typename T, typename Significand>
constexpr auto trig_reduce(const Significand m, const int e, const bool xneg) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};
    constexpr int window {trig_traits<T>::window};
    constexpr int frac {trig_traits<T>::frac};
    static_assert(window - 2 + std::numeric_limits<T>::max_exponent10 / 9 < 720, "2/pi must cover the largest exponent");

    // m * 10^e = (m * 10^shift) * 10^(9 * word_exp)
    const int shift {((e % 9) + 9) % 9};
    const int word_exp {(e - shift) / 9};
    std::uint64_t m_words[6] {};
    const int count {scaled_words(m, shift, m_words)};

    std::uint64_t prod[static_cast<std::size_t>(window)] {};
    times_two_over_pi<window>(m_words, count, word_exp, prod);

    // The fraction, high word first. Above one half, go to the next quadrant with r < 0.
    std::uint64_t f[static_cast<std::size_t>(window - 1)] {};
    for (int i {}; i < window - 1; ++i)
    {
        f[i] = prod[window - 2 - i];
    }
    unsigned n {static_cast<unsigned>(prod[window - 1] & 3U)};
    bool rneg {fold_half<window - 1>(f)};
    if (rneg)
    {
        n = (n + 1U) & 3U;
    }

    // -x = (-n)*pi/2 + (-r)
    if (xneg)
    {
        n = (4U - n) & 3U;
        rneg = !rneg;
    }

    trig_arg<words> out {};
    out.n = n;
    out.neg = rneg;

    // The fraction is not zero: its worst case has 9, 19 and 37 leading zero digits (see trig_traits).
    int lead {};
    while (lead < window - 1 && f[lead] == 0U)
    {
        ++lead;
    }

    std::uint64_t r[static_cast<std::size_t>(2 * frac + 1)] {};
    times_half_pi<frac, window - 1>(f, lead, r);

    // Word 0 of r has the weight 10^-9(lead + 2 * frac).
    int exp10 {};
    out.sig = first_38_digits(r, 2 * frac + 1, exp10);
    out.k = 9 * (lead + 2 * frac) - exp10;
    return out;
}

// a * b + x + carry: returns the low word and puts the high word in carry.
constexpr auto mul_add(const std::uint64_t a, const std::uint64_t b, const std::uint64_t x, std::uint64_t& carry) noexcept -> std::uint64_t
{
    std::uint64_t hi {};
    std::uint64_t lo {boost::int128::detail::umul(a, b, hi)};
    lo += x;
    hi += lo < x ? 1U : 0U;
    lo += carry;
    hi += lo < carry ? 1U : 0U;
    carry = hi;
    return lo;
}

// a[0..count) * v, low word first, in place; returns the new count.
constexpr auto times_word(std::uint64_t* a, int count, const std::uint64_t v) noexcept -> int
{
    std::uint64_t carry {};
    for (int i {}; i < count; ++i)
    {
        a[i] = mul_add(a[i], v, 0U, carry);
    }
    if (carry != 0U)
    {
        a[count++] = carry;
    }
    return count;
}

// x = m * 10^e < 10^19 as Frac + 1 words, low word first, with word Frac as the integer part.
// For e < 0, x = (m * 10^b) * 10^(-9a) with 9a = b - e, and the table error is below 2^-300.
template <int Frac, typename Significand>
constexpr auto binary_words(const Significand m, const int e, std::uint64_t* x) noexcept -> void
{
    constexpr int cw {Frac + 2};
    const auto m128 {static_cast<boost::int128::uint128_t>(m)};
    if (e >= 0)
    {
        x[Frac] = m128.low * pow10(static_cast<std::uint64_t>(e));
        return;
    }

    const int a {(8 - e) / 9};
    std::uint64_t g[4] {m128.low, m128.high};
    const int count {times_word(g, m128.high != 0U ? 2 : 1, pow10(static_cast<std::uint64_t>(9 * a + e)))};

    const auto& c {trig_table::pow10_neg9_bin[a - 1]};
    std::uint64_t p[static_cast<std::size_t>(cw + 3)] {};
    for (int i {}; i < count; ++i)
    {
        std::uint64_t carry {};
        for (int j {}; j < cw; ++j)
        {
            p[i + j] = mul_add(g[i], c[cw - 1 - j], p[i + j], carry);
        }
        p[i + cw] = carry;
    }
    for (int i {}; i <= Frac; ++i)
    {
        x[i] = p[cw - Frac + i];
    }
}

// Payne-Hanek reduction in words of 64 bits for |x| < 10^19. The relative errors of r are below 2^-64,
// 2^-128 and 2^-190, less than those of trig_reduce.
template <typename T, typename Significand>
constexpr auto trig_reduce_binary(const Significand m, const int e, const bool xneg) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};
    constexpr int frac {trig_traits<T>::bin_frac};
    constexpr int tw {frac + 2};
    constexpr int rw {words + 1};

    std::uint64_t x[static_cast<std::size_t>(frac + 1)] {};
    binary_words<frac>(m, e, x);
    trig_arg<words> out {};

    // q = x * 2/pi with tw words of 2/pi: word 2 * frac + 2 is the integer part. The columns below
    // frac only carry into words that are dropped.
    std::uint64_t q[static_cast<std::size_t>(2 * frac + 3)] {};
    for (int i {}; i <= frac; ++i)
    {
        std::uint64_t carry {};
        for (int j {frac - i < 0 ? 0 : frac - i}; j < tw; ++j)
        {
            q[i + j] = mul_add(x[i], trig_table::two_over_pi_bin[tw - 1 - j], q[i + j], carry);
        }
        q[i + tw] = carry;
    }

    // The fraction, high word first. Above one half, go to the next quadrant with r = -(1 - f).
    std::uint64_t f[static_cast<std::size_t>(frac)] {};
    for (int i {}; i < frac; ++i)
    {
        f[i] = q[2 * frac + 1 - i];
    }
    unsigned n {static_cast<unsigned>(q[2 * frac + 2] & 3U)};
    bool rneg {(f[0] >> 63U) != 0U};
    if (rneg)
    {
        std::uint64_t carry {1};
        for (int i {frac - 1}; i >= 0; --i)
        {
            f[i] = ~f[i] + carry;
            carry = (carry != 0U && f[i] == 0U) ? 1U : 0U;
        }
        n = (n + 1U) & 3U;
    }
    if (xneg)
    {
        n = (4U - n) & 3U;
        rneg = !rneg;
    }
    out.n = n;
    out.neg = rneg;

    // The fraction is not zero: its worst case has 27, 58 and 117 leading zero bits (see trig_traits).
    int lead {};
    while (lead < frac && f[lead] == 0U)
    {
        ++lead;
    }

    // r = (rw words of f from word lead) * pi/2 = (s from word rw) * 2^-64(lead + rw). The four spare
    // words take the scale by 10^k below.
    std::uint64_t s[static_cast<std::size_t>(2 * rw + 6)] {};
    for (int u {}; u < rw; ++u)
    {
        const int i {lead + rw - 1 - u};
        const std::uint64_t a {i < frac ? f[i] : 0U};
        std::uint64_t carry {};
        for (int v {}; v <= rw; ++v)
        {
            s[u + v] = mul_add(a, trig_table::pio2_bin[rw - v], s[u + v], carry);
        }
        s[u + rw + 1] = carry;
    }
    std::uint64_t* const r {s + rw};

    // rf = r * 2^(64N - 2): drop lead + 1 words and 2 bits.
    for (int i {}; i < words; ++i)
    {
        const int w {i + lead + 1};
        const std::uint64_t lo {w <= rw ? r[w] : 0U};
        const std::uint64_t hi {w + 1 <= rw ? r[w + 1] : 0U};
        out.rf.w[i] = (lo >> 2U) | (hi << 62U);
    }
    out.fixed = true;

    // For r in [2^-(z + 1), 2^-z), sig = floor(r * 10^k) with k = 38 + floor(z * log10(2)) has 37 or 38 digits.
    int count {rw + 1};
    while (r[count - 1] == 0U)
    {
        --count;
    }
    const int z {64 * (lead + rw - count) + countl_zero(r[count - 1])};
    int k {38 + ((z * 78913) >> 18)};
    int left {k};
    while (left >= 19)
    {
        count = times_word(r, count, pow10(static_cast<std::uint64_t>(19)));
        left -= 19;
    }
    count = times_word(r, count, pow10(static_cast<std::uint64_t>(left)));
    const int unit {lead + rw};
    out.sig = boost::int128::uint128_t {unit + 1 < count ? r[unit + 1] : 0U, unit < count ? r[unit] : 0U};
    if (out.sig < pow10(static_cast<boost::int128::uint128_t>(37)))
    {
        out.sig *= 10U;
        ++k;
    }
    out.k = k;
    return out;
}

// x = sig * 10^-k with sig of 38 digits; for |x| <= pi/4 this is r itself.
template <typename T>
constexpr auto trig_prepare(const T x) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};

    // floor(pi/4 * 10^38) = 78539816339744830961566084581987572104
    constexpr boost::int128::uint128_t quarter_pi {UINT64_C(4257651975108193236), UINT64_C(12340784068543502728)};

    trig_arg<words> out {};
    const bool neg {signbit(x)};
    int e {};
    const auto m {frexp10(neg ? -x : x, &e)};
    out.neg = neg;
    if (m == 0U)
    {
        out.zero = true;
        return out;
    }

    const auto digits {num_digits(m)};
    const auto sig {static_cast<boost::int128::uint128_t>(m) * pow10(static_cast<boost::int128::uint128_t>(38 - digits))};
    const int k {(38 - digits) - e};
    if (k > 38 || (k == 38 && sig <= quarter_pi))
    {
        out.sig = sig;
        out.k = k;
        return out;
    }
    // |x| < 10^digits * 10^e
    if (digits + e <= 19)
    {
        return trig_reduce_binary<T>(m, e, neg);
    }
    return trig_reduce<T>(m, e, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP
