// Copyright 2023 - 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FORMAT_HPP
#define BOOST_DECIMAL_FORMAT_HPP

// The feature-test macros below are only visible once <version> has been seen.
// The module build gets them from the global module fragment instead.
#ifndef BOOST_DECIMAL_BUILD_MODULE
#  if __has_include(<version>)
#    include <version>
#  endif
#endif

#if __has_include(<format>) && defined(__cpp_lib_format) && __cpp_lib_format >= 201907L && !defined(BOOST_DECIMAL_DISABLE_CLIB)

#define BOOST_DECIMAL_HAS_FORMAT_SUPPORT

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/io.hpp>
#include <boost/decimal/detail/format_render.hpp>
#include <boost/decimal/charconv.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE

#include <format>
#include <sstream>
#include <type_traits>

#endif // BOOST_DECIMAL_BUILD_MODULE

// Default :g
// Fixed :f
// Scientific :e
// Hex :x
// Cohort Preserving :a
//
// Capital letter for any of the above leads to all characters being uppercase

namespace boost::decimal::detail {

template <typename>
struct formattable_character_type : std::false_type {};

template <>
struct formattable_character_type<char> : std::true_type {};

template <>
struct formattable_character_type<wchar_t> : std::true_type {};

template <typename CharT>
inline constexpr bool is_formattable_character_type_v = formattable_character_type<CharT>::value;

} // Namespace boost::decimal::detail

namespace std {

template <boost::decimal::detail::concepts::decimal_floating_point_type T, typename CharT>
    requires boost::decimal::detail::is_formattable_character_type_v<CharT>
struct formatter<T, CharT>
{
    boost::decimal::detail::format_spec<CharT> spec;

    constexpr auto parse(basic_format_parse_context<CharT>& ctx)
    {
        return boost::decimal::detail::parse_format_spec<std::format_error>(ctx.begin(), ctx.end(), spec, !boost::decimal::detail::is_fast_type_v<T>);
    }

    template <typename FormatContext>
    auto format(const T &v, FormatContext &ctx) const
    {
        using namespace boost::decimal;
        using namespace boost::decimal::detail;

        using CharType = FormatContext::char_type;
        static_assert(is_formattable_character_type_v<CharType>, "This is an unsupported character type. Only char and wchar_t can be used with std::format");

        const auto localize = [&ctx](char* first, char* last)
        {
            std::basic_ostringstream<CharType> os;
            os.imbue(ctx.locale());
            put_chars(os, first, last);
            return os.str();
        };

        return format_decimal<std::format_error>(v, spec, ctx.out(), localize);
    }
};

} // Namespace std

#endif // <format> compatibility

#endif //BOOST_DECIMAL_FORMAT_HPP
