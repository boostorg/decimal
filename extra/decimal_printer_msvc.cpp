// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Visual Studio legacy addin used by decimal_printer_msvc.natvis.
// NATVIS on its own can not decode a decimal encoding, so the value is read out of the
// debuggee and handed to the library's own to_chars, which keeps the display in agreement
// with the GDB and LLDB pretty printers in this folder.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include <boost/decimal.hpp>
#include <boost/decimal/detail/u256.hpp>

#include <cassert>
#include <cstddef>
#include <cstring>

#define my_min(a, b) ((b)<(a)?(b):(a))


#define arch_k_arm64ec 1001
#define arch_k_arm64   1002
#define arch_k_arm32   1003
#define arch_k_mips    1004
#define arch_k_ppc     1005
#define arch_k_alpha   1006
#define arch_k_ia64    1007
#define arch_k_x8664   1008
#define arch_k_x8632   1009

#if 0
#elif defined _MSC_VER && defined _M_X64 && defined _M_AMD64 && defined _M_ARM64EC
#define arch_v arch_k_arm64ec
#elif defined _MSC_VER && defined _M_ARM64
#define arch_v arch_k_arm64
#elif defined _MSC_VER && defined _M_ARM
#define arch_v arch_k_arm32
#elif defined _MSC_VER && defined _M_MRX000
#define arch_v arch_k_mips
#elif defined _MSC_VER && (defined _M_PPC || defined _M_MPPC)
#define arch_v arch_k_ppc
#elif defined _MSC_VER && defined _M_ALPHA
#define arch_v arch_k_alpha
#elif defined _MSC_VER && defined _M_IA64
#define arch_v arch_k_ia64
#elif defined _MSC_VER && defined _M_X64 && defined _M_AMD64
#define arch_v arch_k_x8664
#elif defined _MSC_VER && defined _M_IX86 && !defined _M_I86 && !defined M_I86
#define arch_v arch_k_x8632
#elif defined _MSC_VER && (defined _M_I86 || defined M_I86)
#define arch_v arch_k_x8616
#elif defined _WIN64
#define arch_v arch_k_x8664
#elif defined _WIN32
#define arch_v arch_k_x8632
#else
#error unknown arch
#endif

#define arch_is_arm64ec (arch_v == arch_k_arm64ec)
#define arch_is_arm64   (arch_v == arch_k_arm64  )
#define arch_is_arm32   (arch_v == arch_k_arm32  )
#define arch_is_mips    (arch_v == arch_k_mips   )
#define arch_is_ppc     (arch_v == arch_k_ppc    )
#define arch_is_alpha   (arch_v == arch_k_alpha  )
#define arch_is_ia64    (arch_v == arch_k_ia64   )
#define arch_is_x8664   (arch_v == arch_k_x8664  )
#define arch_is_x8632   (arch_v == arch_k_x8632  )


struct DEBUGHELPER_s;
typedef struct DEBUGHELPER_s DEBUGHELPER_t;

struct DEBUGHELPER_s
{
    DWORD m_version;
    HRESULT(WINAPI*m_pfn_ReadDebuggeeMemory)(DEBUGHELPER_t* const self, DWORD const addr, DWORD const want, void* const where, DWORD* const got);
    DWORDLONG(WINAPI*m_pfn_GetRealAddress)(DEBUGHELPER_t* const self);
    HRESULT(WINAPI*m_pfn_ReadDebuggeeMemoryEx)(DEBUGHELPER_t* const self, DWORDLONG const addr, DWORD const want, void* const where, DWORD* const got);
    int(WINAPI*m_pfn_GetProcessorType)(DEBUGHELPER_t* const self);
};
typedef struct DEBUGHELPER_s DEBUGHELPER_t;

typedef HRESULT(WINAPI*customviewer_t)(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);


namespace {

// Big enough for every type here. The widest finite decimal128_t rendering is roughly
// forty characters, and every writer below is bounded by the end pointer it is given.
constexpr std::size_t text_buffer_size {128U};

static_assert(text_buffer_size > boost::decimal::formatting_limits<boost::decimal::decimal128_t>::cohort_preserving_scientific_max_chars,
              "The scratch buffer must hold the widest cohort preserving rendering");

// The fast types store a significand that is already normalized, so they have no cohorts of
// their own. Rendering them through the IEEE type of the same width reuses to_chars without
// disturbing the significand and exponent that were read out of the debuggee.
template <typename T>
struct cohort_type
{
    using type = T;
};

template <>
struct cohort_type<boost::decimal::decimal_fast32_t>
{
    using type = boost::decimal::decimal32_t;
};

template <>
struct cohort_type<boost::decimal::decimal_fast64_t>
{
    using type = boost::decimal::decimal64_t;
};

template <>
struct cohort_type<boost::decimal::decimal_fast128_t>
{
    using type = boost::decimal::decimal128_t;
};

// Spells non-finite values the way the GDB and LLDB printers do, so all three debuggers
// agree. Returns the end of the text, or nullptr if the buffer was too small.
template <typename T>
auto write_nonfinite(const T& value, char* first, char* const last) noexcept -> char*
{
    if (boost::decimal::signbit(value))
    {
        *first++ = '-';
    }

    if (boost::decimal::isinf(value))
    {
        std::memcpy(first, "INF", 3U);
        return first + 3;
    }

    std::memcpy(first, boost::decimal::issignaling(value) ? "SNAN" : "QNAN", 4U);
    first += 4;

    const auto payload {boost::decimal::read_payload(value)};
    if (payload != 0U)
    {
        *first++ = '(';

        // One character is held back so the closing parenthesis always has room
        const auto r {boost::decimal::detail::to_chars_integer_impl(first, last - 1, payload)};
        if (!r)
        {
            return nullptr;
        }

        first = r.ptr;
        *first++ = ')';
    }

    return first;
}

// Renders one decimal value. Returns the length written, or a negative value on failure.
template <typename T>
auto format_decimal(const T& value, char* const first, char* const last) noexcept -> std::ptrdiff_t
{
    if (!boost::decimal::isfinite(value))
    {
        const auto end {write_nonfinite(value, first, last)};
        return end == nullptr ? -1 : end - first;
    }

    using ieee_type = typename cohort_type<T>::type;

    const auto r {boost::decimal::to_chars(first, last, static_cast<ieee_type>(value),
                                           boost::decimal::chars_format::cohort_preserving_scientific)};

    return r ? r.ptr - first : -1;
}

// Renders u256 in decimal with a comma every three digits, matching the GDB and LLDB printers.
auto format_u256(const boost::decimal::detail::u256& value, char* const first, char* const last) noexcept -> std::ptrdiff_t
{
    char digits[128] {};
    const auto* const text {boost::decimal::detail::impl::u256_to_buffer(digits, value)};
    const auto length {static_cast<std::ptrdiff_t>(std::strlen(text))};

    if (last - first < length + (length - 1) / 3)
    {
        return -1;
    }

    auto* out {first};
    for (std::ptrdiff_t i {}; i != length; ++i)
    {
        if (i != 0 && (length - i) % 3 == 0)
        {
            *out++ = ',';
        }

        *out++ = text[i];
    }

    return out - first;
}

// Copies sizeof(T) bytes of the value out of the process being debugged.
template <typename T>
auto read_debuggee(DWORD const address, DEBUGHELPER_t* const helper, T& value) noexcept -> HRESULT
{
    static_cast<void>(address);

    DWORD got {};
    HRESULT hr {};

    #if arch_is_x8664
    assert(helper->m_version >= 0x00020000);
    assert(helper->m_pfn_GetRealAddress);
    assert(helper->m_pfn_ReadDebuggeeMemoryEx);
    const auto real_address {helper->m_pfn_GetRealAddress(helper)};
    hr = helper->m_pfn_ReadDebuggeeMemoryEx(helper, real_address, sizeof(T), &value, &got);
    #elif arch_is_x8632
    assert(helper->m_pfn_ReadDebuggeeMemory);
    hr = helper->m_pfn_ReadDebuggeeMemory(helper, address, sizeof(T), &value, &got);
    #else
    #error unsupported arch
    #endif

    if (hr != S_OK)
    {
        return hr;
    }

    return got == sizeof(T) ? S_OK : E_FAIL;
}

// Writes the finished text into the debugger's buffer, narrow or wide, always terminated.
void copy_result(const char* const text, std::size_t const length, BOOL const unicode, char* const result, std::size_t const maximum) noexcept
{
    if (maximum == 0)
    {
        return;
    }

    const std::size_t n {my_min(length, maximum - 1U)};

    if (unicode)
    {
        auto* const wide {reinterpret_cast<wchar_t*>(result)};
        for (std::size_t i {}; i != n; ++i)
        {
            wide[i] = static_cast<wchar_t>(text[i]);
        }

        wide[n] = L'\0';
    }
    else
    {
        for (std::size_t i {}; i != n; ++i)
        {
            result[i] = text[i];
        }

        result[n] = '\0';
    }
}

template <typename T>
auto formatter_impl(DWORD const address, DEBUGHELPER_t* const helper, char* const result, std::size_t const maximum, BOOL const unicode) noexcept -> HRESULT
{
    assert(helper);
    assert(result || maximum == 0);

    T value {};
    const auto hr {read_debuggee(address, helper, value)};
    if (hr != S_OK)
    {
        return hr;
    }

    char text[text_buffer_size] {};
    const auto length {format_decimal(value, text, text + text_buffer_size)};
    if (length < 0)
    {
        return E_FAIL;
    }

    copy_result(text, static_cast<std::size_t>(length), unicode, result, maximum);

    return S_OK;
}

auto formatter_u256_impl(DWORD const address, DEBUGHELPER_t* const helper, char* const result, std::size_t const maximum, BOOL const unicode) noexcept -> HRESULT
{
    assert(helper);
    assert(result || maximum == 0);

    boost::decimal::detail::u256 value {};
    const auto hr {read_debuggee(address, helper, value)};
    if (hr != S_OK)
    {
        return hr;
    }

    char text[text_buffer_size] {};
    const auto length {format_u256(value, text, text + text_buffer_size)};
    if (length < 0)
    {
        return E_FAIL;
    }

    copy_result(text, static_cast<std::size_t>(length), unicode, result, maximum);

    return S_OK;
}

} // namespace


extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal32(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal64(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal128(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal_fast32(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal_fast64(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_decimal_fast128(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);
extern "C" __declspec(dllexport) HRESULT __stdcall formatter_u256(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved);


__declspec(dllexport) HRESULT __stdcall formatter_decimal32(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal32_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_decimal64(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal64_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_decimal128(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal128_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_decimal_fast32(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal_fast32_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_decimal_fast64(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal_fast64_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_decimal_fast128(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_impl<boost::decimal::decimal_fast128_t>(address, helper, result, maximum, unicode);
}

__declspec(dllexport) HRESULT __stdcall formatter_u256(DWORD const address, DEBUGHELPER_t* const helper, int const base, BOOL const unicode, char* const result, size_t const maximum, DWORD const reserved)
{
    static_cast<void>(base);
    static_cast<void>(reserved);
    return formatter_u256_impl(address, helper, result, maximum, unicode);
}

BOOL APIENTRY DllMain(HMODULE const hmodule, DWORD const reason, LPVOID const reserved)
{
    static_cast<void>(hmodule);
    static_cast<void>(reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH: break;
        case DLL_THREAD_ATTACH : break;
        case DLL_THREAD_DETACH : break;
        case DLL_PROCESS_DETACH: break;
        default                : break;
    }

    return TRUE;
}
