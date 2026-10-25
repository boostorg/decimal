// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1533

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <locale>
#include <limits>
#include <new>
#include <sstream>
#include <string>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The largest array that the program asks for, so that a test can find a buffer that the precision makes too large.
// The standard library can also ask for small arrays
std::size_t largest_array {};

void* operator new[](std::size_t size)
{
    largest_array = size > largest_array ? size : largest_array;
    void* p {std::malloc(size == 0U ? 1U : size)};
    if (p == nullptr)
    {
        std::abort();
    }
    return p;
}

void operator delete[](void* p) noexcept
{
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept
{
    std::free(p);
}

// Counts the characters that the stream widens
struct counting_ctype : std::ctype<wchar_t>
{
    static std::size_t count;
    char_type do_widen(char c) const override
    {
        ++count;
        return std::ctype<wchar_t>::do_widen(c);
    }
    const char* do_widen(const char* first, const char* last, char_type* dest) const override
    {
        count += static_cast<std::size_t>(last - first);
        return std::ctype<wchar_t>::do_widen(first, last, dest);
    }
};

std::size_t counting_ctype::count {};

// Takes no characters
struct full_wide_buffer : std::wstreambuf
{
};

// Groups by three and counts the punctuation that the stream asks for
struct counting_numpunct : std::numpunct<wchar_t>
{
    static std::size_t count;
    std::string do_grouping() const override { return "\3"; }
    char_type do_thousands_sep() const override
    {
        ++count;
        return L',';
    }
    char_type do_decimal_point() const override
    {
        ++count;
        return L'.';
    }
};

std::size_t counting_numpunct::count {};

// The expected strings are what std::ostream gives for the same double
template <typename T, typename Manipulator>
std::string print(const T value, Manipulator manipulator)
{
    std::ostringstream out;
    manipulator(out);
    out << value;
    return out.str();
}

// A numpunct with the Indian grouping, as hi_IN, so that the test needs no installed locale
template <typename charT>
struct indian_numpunct : std::numpunct<charT>
{
    std::string do_grouping() const override { return "\3\2"; }
    charT do_decimal_point() const override { return static_cast<charT>(','); }
    charT do_thousands_sep() const override { return static_cast<charT>('.'); }
};

#if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS)
struct output_error {};

struct throwing_buffer : std::streambuf
{
    int_type overflow(int_type) override { throw output_error {}; }
};
#endif

// Takes only the first 20 characters
struct short_buffer : std::streambuf
{
    char data[20] {};
    short_buffer() { setp(data, data + sizeof(data)); }
};

template <typename T>
void test()
{
    const T x {1.5_DL};
    BOOST_TEST_EQ(print(T{1e-18_DL}, [](std::ostream& o) { o << std::fixed << std::setprecision(20); }), "0.00000000000000000100");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::hex << 255 << ' '; }), "ff 1.5");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::uppercase << std::scientific; }), "1.500000E+00");
    BOOST_TEST_EQ(print(std::numeric_limits<T>::infinity(), [](std::ostream& o) { o << std::uppercase; }), "INF");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::showpos; }), "+1.5");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::showpoint; }), "1.50000");
    BOOST_TEST_EQ(print(T{2_DL}, [](std::ostream& o) { o << std::showpoint << std::fixed << std::setprecision(0); }), "2.");
    BOOST_TEST_EQ(print(T{2_DL}, [](std::ostream& o) { o << std::showpoint << std::scientific << std::setprecision(0); }), "2.e+00");
    BOOST_TEST_EQ(print(-x, [](std::ostream& o) { o << std::internal << std::setfill('*') << std::setw(8); }), "-****1.5");
    BOOST_TEST_EQ(print(-x, [](std::ostream& o) { o << std::left << std::setfill('*') << std::setw(8); }), "-1.5****");

    // A negative precision is 6, as for printf
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::showpoint << std::setprecision(-1); }), "1.50000");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::fixed << std::setprecision(-1); }), "1.500000");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::scientific << std::setprecision(-1); }), "1.500000e+00");

    // hexfloat is fixed | scientific, gives the hex format and ignores the precision
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::hexfloat << std::setprecision(1); }), "fp-01");
    BOOST_TEST_EQ(print(T{1_DL}, [](std::ostream& o) { o << std::hexfloat << std::showpoint; }), "1.p+00");

    // hexfloat, inf and NaN do not use the precision, so a large precision gives no large buffer
    using limits = std::numeric_limits<T>;
    std::wostringstream wide_inf;
    largest_array = 0U;
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::hexfloat << std::setprecision(1000000); }), "fp-01");
    BOOST_TEST_EQ(print(x, [](std::ostream& o) { o << std::hexfloat << std::showpoint << std::setprecision(1000000); }), "f.p-01");
    BOOST_TEST_EQ(print(limits::infinity(), [](std::ostream& o) { o << std::setprecision(1000000); }), "inf");
    BOOST_TEST_EQ(print(-limits::infinity(), [](std::ostream& o) { o << std::fixed << std::setprecision(1000000); }), "-inf");
    BOOST_TEST_EQ(print(limits::quiet_NaN(), [](std::ostream& o) { o << std::showpoint << std::setprecision(1000000); }), "nan");
    BOOST_TEST_EQ(print(limits::quiet_NaN(), [](std::ostream& o) { o << std::showpoint << std::fixed << std::setprecision(1000000); }), "nan");
    wide_inf << std::scientific << std::setprecision(1000000) << limits::infinity();
    BOOST_TEST(wide_inf.str() == L"inf");
    BOOST_TEST_LT(largest_array, 4096U);

    // Results just below, at and just above the 512 characters of the local buffer
    for (int precision {500}; precision < 516; ++precision)
    {
        const auto size {static_cast<std::size_t>(precision)};
        BOOST_TEST_EQ(print(-x, [&](std::ostream& o) { o << std::scientific << std::setprecision(precision); }).size(), size + 7U);
        BOOST_TEST_EQ(print(T{-15, -6}, [&](std::ostream& o) { o << std::showpoint << std::setprecision(precision); }).size(), size + 6U);
        BOOST_TEST_EQ(print(x, [&](std::ostream& o) { o << std::showpoint << std::setprecision(precision); }).size(), size + 1U);
    }

    std::wostringstream wide;
    wide << std::showpos << std::setw(6) << x;
    BOOST_TEST(wide.str() == L"  +1.5");

    // The numpunct of the stream character type groups only the digits before the point
    const std::locale indian {std::locale::classic(), new indian_numpunct<char>};
    BOOST_TEST_EQ(print(T{1234567_DL}, [&](std::ostream& o) { o.imbue(indian); o << std::fixed << std::setprecision(1); }), "12.34.567,0");
    BOOST_TEST_EQ(print(T{123456_DL}, [&](std::ostream& o) { o.imbue(indian); o << std::scientific << std::setprecision(0); }), "1e+05");
    std::wostringstream wide_grouped;
    wide_grouped.imbue(std::locale(std::locale::classic(), new indian_numpunct<wchar_t>));
    wide_grouped << std::fixed << std::setprecision(1) << T{98765_DL};
    BOOST_TEST(wide_grouped.str() == L"98.765,0");

    // An exception during the output sets badbit, and goes to the caller only with badbit in the mask
    #if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS)
    throwing_buffer buffer_that_throws;
    std::ostream quiet {&buffer_that_throws};
    BOOST_TEST_NO_THROW(quiet << x);
    BOOST_TEST(quiet.bad());
    std::ostream loud {&buffer_that_throws};
    loud.exceptions(std::ios_base::badbit);
    loud.width(10);
    BOOST_TEST_THROWS(loud << x, output_error);
    BOOST_TEST(loud.bad());
    BOOST_TEST_EQ(loud.width(), 0);
    std::ostream loud_long {&buffer_that_throws};
    loud_long.exceptions(std::ios_base::badbit);
    BOOST_TEST_THROWS(loud_long << std::fixed << std::setprecision(40) << x, output_error);
    BOOST_TEST(loud_long.bad());
    #endif

    // A stream buffer that takes only a part of a long run sets badbit
    short_buffer partial;
    std::ostream cut {&partial};
    cut << std::fixed << std::setprecision(40) << x;
    BOOST_TEST(cut.bad());

    // The output stops at the first failed write
    full_wide_buffer full;
    std::wostream stopped {&full};
    stopped.imbue(std::locale(std::locale::classic(), new counting_ctype));
    counting_ctype::count = 0U;
    stopped << std::fixed << std::setprecision(40) << std::setw(1000000) << std::left << x;
    BOOST_TEST(stopped.bad());
    BOOST_TEST_LE(counting_ctype::count, 1U);

    // A grouped value that fails at its first digit asks for no separator and no point
    std::wostream stopped_grouped {&full};
    stopped_grouped.imbue(std::locale(std::locale(std::locale::classic(), new counting_ctype), new counting_numpunct));
    counting_ctype::count = 0U;
    counting_numpunct::count = 0U;
    stopped_grouped << std::fixed << std::setprecision(1) << T{1234567_DL};
    BOOST_TEST(stopped_grouped.bad());
    BOOST_TEST_LE(counting_ctype::count, 1U);
    BOOST_TEST_EQ(counting_numpunct::count, 0U);
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    // A value longer than the old 1024 character buffer
    std::ostringstream out;
    out << std::fixed << std::numeric_limits<decimal128_t>::max();
    BOOST_TEST_EQ(out.str().size(), 6152U);

    // A wide stream keeps the characters after the grouping separators
    #if !(defined(__APPLE__) && defined(__GNUC__) && !defined(__clang__)) && !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS) && !defined(BOOST_DECIMAL_QEMU_TEST)
    try
    {
        std::wostringstream grouped;
        grouped.imbue(std::locale("en_US.UTF-8"));
        grouped << std::fixed << std::setprecision(1) << 1234567.5_DD;
        BOOST_TEST(grouped.str() == L"1,234,567.5");
    }
    catch (...)
    {
        std::cerr << "Locale not installed. Skipping test." << std::endl;
    }
    #endif

    return boost::report_errors();
}
