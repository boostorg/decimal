// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1532

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
#include <new>
#include <string>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The cstdio functions get their arrays with the nothrow new. A test can make the array with
// this number fail, and can count the bytes that the arrays ask for
int failing_array {};
std::size_t array_bytes {};

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    array_bytes += size;
    if (failing_array > 0 && --failing_array == 0)
    {
        return nullptr;
    }

    // As the default nothrow form does, this gets the array from the default operator new[]
    #if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS)
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return nullptr;
    }
    #else
    return ::operator new[](size);
    #endif
}

// MSVC deprecates std::tmpfile, and gives tmpfile_s in its place
std::FILE* open_temporary_file()
{
    #if defined(_MSC_VER)
    std::FILE* file {};
    return tmpfile_s(&file) == 0 ? file : nullptr;
    #else
    return std::tmpfile();
    #endif
}

// A numpunct with the Indian grouping and a comma point, so that the test needs no installed locale
struct indian_numpunct : std::numpunct<char>
{
    std::string do_grouping() const override { return "\3\2"; }
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '.'; }
};

// The expected strings are what std::snprintf gives for the same double
template <typename... T>
void check(const char* expected, const char* format, const T... values)
{
    char buffer[64];
    const auto n {boost::decimal::snprintf(buffer, sizeof(buffer), format, values...)};
    BOOST_TEST_CSTR_EQ(buffer, expected);
    BOOST_TEST_EQ(n, static_cast<int>(std::strlen(expected)));
}

void test_format_text()
{
    check("1.5,2.5|", "%Dg,%Dg|", 1.5_DD, 2.5_DD);
    check("1.5\n", "%Dg\n", 1.5_DD);
    check("x%y 1.5", "x%%y %Dg", 1.5_DD);
    check("INF NAN INF", "%DE %DG %DF", std::numeric_limits<decimal64_t>::infinity(),
          std::numeric_limits<decimal64_t>::quiet_NaN(), std::numeric_limits<decimal64_t>::infinity());
}

void test_flags_and_width()
{
    check("      3.14", "%10.2Df", 3.14159_DD);
    check("3.1     |", "%-8.1Df|", 3.14159_DD);
    check("+3.1", "%+.1Df", 3.14159_DD);
    check(" 3.1", "% .1Df", 3.14159_DD);
    check("-0003.14", "%08.2Df", -3.14159_DD);
    check("+0003.14", "%+08.2Df", 3.14159_DD);
    check("3.14    ", "%-08.2Df", 3.14159_DD);
    check("    -inf", "%08Df", -std::numeric_limits<decimal64_t>::infinity());
    check("  1.5e+00", "%9.1He", 1.5_DF);
    check("3", "%.0DDf", 3.14159_DL);
    check("fp-01", "%Da", 1.5_DD);
    check("f.000p-01", "%.3Da", 1.5_DD);

    // A hex significand can start with a letter, and it still pads with zeros
    check("000fp-01", "%08Da", 1.5_DD);
    check("+000FP-01", "%+09DA", 1.5_DD);
    check("-000fp-01", "%09Da", -1.5_DD);
}

void test_truncation()
{
    char buffer[20];
    std::memset(buffer, 'x', sizeof(buffer));

    // The output does not fit: write what fits, and return the full length
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, 4, "%Df", 123456.0_DD), 13);
    BOOST_TEST_CSTR_EQ(buffer, "123");
    BOOST_TEST_EQ(buffer[4], 'x');

    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%Df %Df", 123456.0_DD, 123456.0_DD), 27);
    BOOST_TEST_CSTR_EQ(buffer, "123456.000000 12345");

    BOOST_TEST_EQ(boost::decimal::snprintf(nullptr, 0, "%Df", 123456.0_DD), 13);

    // A value longer than the internal buffer
    const auto large {std::numeric_limits<decimal128_t>::max()};
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%.2DDf", large), 6148);
    BOOST_TEST_CSTR_EQ(buffer, "9999999999999999999");

    // A conversion with no value
    errno = 0;
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%Dg %Dg", 1.5_DD), -1);
    BOOST_TEST_EQ(errno, EINVAL);
}

// inf and nan do not use the precision, so it does not size a buffer or fail the call
void test_large_precision()
{
    // The general format keeps only the digits of the type, so a large precision needs no large buffer
    char one[4];
    array_bytes = 0U;
    BOOST_TEST_EQ(boost::decimal::snprintf(one, sizeof(one), "%.1000000Dg", 1.0_DD), 1);
    BOOST_TEST_CSTR_EQ(one, "1");
    BOOST_TEST_EQ(array_bytes, 0U);

    char buffer[8];
    const auto inf {std::numeric_limits<decimal64_t>::infinity()};
    array_bytes = 0U;
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%.1000000Df", inf), 3);
    BOOST_TEST_EQ(array_bytes, 0U);
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, 2, "%.2147483647Df", inf), 3);
    BOOST_TEST_CSTR_EQ(buffer, "i");
}

// The type modifier can narrow a finite value to inf, which pads with spaces
void test_narrowing()
{
    const auto large {std::numeric_limits<decimal128_t>::max()};
    check("     inf", "%08Hg", large);
    check("    +inf", "%+08Hg", large);
    check("    -inf", "%08Hg", -large);
    check("     INF", "%08HG", large);
    check("     inf", "%08Df", large);

    // The cast value also sizes the buffer, so the precision of a value that narrows to inf is not used
    const decimal128_t above_d32 {1, 100};
    check("inf", "%.2147483647Hf", above_d32);
    check("+inf", "%+.2147483647He", above_d32);
    check("-INF", "%.2147483647DE", -large);
    check("-inf", "%.2147483647Df", -large);
    array_bytes = 0U;
    check("inf", "%.1000000Hf", above_d32);
    BOOST_TEST_EQ(array_bytes, 0U);
}

// A width or a precision above INT_MAX fails as printf does
void test_overflow()
{
    char buffer[2];
    errno = 0;
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%2147483648Dg", 1.0_DD), -1);
    BOOST_TEST_EQ(errno, EOVERFLOW);
    errno = 0;
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%.99999999999Df", 1.0_DD), -1);
    BOOST_TEST_EQ(errno, EOVERFLOW);
    errno = 0;
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "%2147483647Dg", 1.0_DD), 2147483647);
    BOOST_TEST_EQ(errno, 0);

    // The full count must fit in an int
    BOOST_TEST_EQ(boost::decimal::snprintf(buffer, sizeof(buffer), "x%2147483647Dg", 1.0_DD), -1);
    BOOST_TEST_EQ(errno, EOVERFLOW);
}

void test_fprintf()
{
    std::FILE* file {open_temporary_file()};
    if (file == nullptr)
    {
        return;
    }

    const auto large {std::numeric_limits<decimal128_t>::max()};
    BOOST_TEST_EQ(boost::decimal::fprintf(file, "%Dg", 1.5_DD), 3);
    BOOST_TEST_EQ(boost::decimal::fprintf(file, "%.2DDf", large), 6148);

    std::string contents(6152, '\0');
    std::rewind(file);
    contents.resize(std::fread(&contents[0], 1, contents.size(), file));
    std::fclose(file);

    BOOST_TEST_EQ(contents.size(), 6151U);
    BOOST_TEST_EQ(contents.substr(0, 7), "1.59999");
    BOOST_TEST_EQ(contents.substr(6146), "00.00");

    // The arrays are the buffer of the first pass, the output, and the buffer of the second pass.
    // When the last one fails, fprintf writes nothing
    file = open_temporary_file();
    if (file == nullptr)
    {
        return;
    }
    failing_array = 3;
    errno = 0;
    BOOST_TEST_EQ(boost::decimal::fprintf(file, "%.1200Df", 1.5_DD), -1);
    BOOST_TEST_EQ(errno, ENOMEM);
    BOOST_TEST_EQ(failing_array, 0);
    failing_array = 0;
    BOOST_TEST_EQ(std::ftell(file), 0L);
    std::fclose(file);
}

// A grouping separator goes only between the digits of the integer part, so an exponent has none
void test_locale()
{
    #if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS) && !defined(BOOST_DECIMAL_QEMU_TEST)
    try
    {
        std::locale::global(std::locale("en_US.UTF-8"));
    }
    catch (...)
    {
        std::cerr << "Test not run" << std::endl;
        return;
    }

    check("1e+06 1,234,567.0 inf", "%.0De %.1Df %Df", 1234567.0_DD, 1234567.0_DD, std::numeric_limits<decimal64_t>::infinity());
    std::locale::global(std::locale::classic());
    #endif
}

// All the group sizes apply, and the point of a hex value follows a letter
void test_custom_locale()
{
    std::locale::global(std::locale(std::locale::classic(), new indian_numpunct));
    check("12.34.567,0", "%.1Df", 1234567.0_DD);
    check("1.23.45.678", "%.0Df", 12345678.0_DD);
    check("1,5", "%Dg", 1.5_DD);
    check("f,000p-01", "%.3Da", 1.5_DD);
    check("1,2d687p+05", "%Da", 1234567.0_DD);
    std::locale::global(std::locale::classic());
}

int main()
{
    test_format_text();
    test_flags_and_width();
    test_truncation();
    test_fprintf();
    test_locale();
    test_custom_locale();
    test_large_precision();
    test_narrowing();
    test_overflow();

    return boost::report_errors();
}
