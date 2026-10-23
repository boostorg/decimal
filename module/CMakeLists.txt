# Copyright 2026 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
#
# Builds Boost.Decimal as a C++ module (boost.decimal) and runs the module-aware
# tests in ../test against it, consuming the standard library module (import std).
# This file is included from ../test/CMakeLists.txt when the CMake option
# BOOST_DECIMAL_BUILD_MODULE is set and expects the Boost superproject (for the
# Boost:: dependency targets). import std is required (see the check below).

if(NOT TARGET tests)
    add_custom_target(tests)
endif()

# The module interface unit.
add_library(boost_decimal_module)
target_sources(boost_decimal_module
        PUBLIC
        FILE_SET decimal_module TYPE CXX_MODULES FILES decimal.cppm
)
target_link_libraries(boost_decimal_module PUBLIC Boost::decimal)
target_compile_features(boost_decimal_module PUBLIC cxx_std_23)

# The purview includes the library's angled <boost/decimal...> headers by design.
# clang-scan-deps warns on that and ignores -Wno-include-angled-in-module-purview,
# so -w keeps the dependency-scan output clean (this glue unit's code is fully
# warned on in the header builds).
target_compile_options(boost_decimal_module PRIVATE $<$<CXX_COMPILER_ID:Clang>:-w>)

# decimal.cppm defines BOOST_DECIMAL_BUILD_MODULE itself for the header export
# logic. The tests link this target and switch to `import boost.decimal;` when
# BOOST_DECIMAL_USE_MODULE is set, so propagate that macro to them.
target_compile_definitions(boost_decimal_module PUBLIC BOOST_DECIMAL_USE_MODULE)

# The tests introspect standard-library facilities (e.g. numeric_limits members) that
# a module consumer only sees reliably through the standard library module, so import
# std is required for the test build. CMAKE_CXX_COMPILER_IMPORT_STD lists the standards
# it is available for; it is only populated when CMAKE_EXPERIMENTAL_CXX_IMPORT_STD is
# set before the project() call.
if(NOT "23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR
            "Building the Boost.Decimal module tests requires 'import std' (the C++23 "
            "standard library module). Use a toolchain that supports it and pass "
            "-DCMAKE_EXPERIMENTAL_CXX_IMPORT_STD=<uuid> before the project() call. "
            "CMAKE_CXX_COMPILER_IMPORT_STD='${CMAKE_CXX_COMPILER_IMPORT_STD}'")
endif()

set_target_properties(boost_decimal_module PROPERTIES CXX_MODULE_STD ON)
target_compile_definitions(boost_decimal_module PRIVATE BOOST_DECIMAL_USE_STD_MODULE)
message(STATUS "Boost.Decimal module: using 'import std;'")

# Tests that switch to `import boost.decimal;` when BOOST_DECIMAL_BUILD_MODULE is set.
set(BOOST_DECIMAL_MODULE_TESTS
        github_issue_448
        github_issue_798
        github_issue_802
        github_issue_805
        github_issue_808
        github_issue_890
        github_issue_900
        github_issue_988
        github_issue_1026
        github_issue_1035
        github_issue_1054
        github_issue_1055
)

# github_issue_1057 is intentionally omitted: it calls the free isnan() on the
# wider decimal types, and clang resolves those overloads differently across the
# module boundary (only the decimal32_t overload stays reachable), which is a
# library-level concern independent of module support. The header suite still
# covers it.

# GCC's libstdc++ cannot yet mix the test sources' textual standard library
# includes with import std in one consumer translation unit, so only the quick
# test runs there. MSVC is treated the same way until its full consumer suite has
# been validated. The full list runs under clang/libc++.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Boost.Decimal module: ${CMAKE_CXX_COMPILER_ID} consumer tests limited to module_quick_test")
    set(BOOST_DECIMAL_MODULE_TESTS)
endif()

add_executable(module_quick_test "${CMAKE_CURRENT_SOURCE_DIR}/quick_test.cpp")
target_link_libraries(module_quick_test PRIVATE boost_decimal_module)
# The superproject pins an older policy version (CMP0155), so C++ sources are not
# scanned for module imports by default. Force scanning so `import boost.decimal;`
# is resolved to the module built above.
set_target_properties(module_quick_test PROPERTIES CXX_SCAN_FOR_MODULES ON CXX_MODULE_STD ON)
add_test(NAME module_quick_test COMMAND module_quick_test)
add_dependencies(tests module_quick_test)

foreach(test ${BOOST_DECIMAL_MODULE_TESTS})
    add_executable(module_${test} "${CMAKE_CURRENT_SOURCE_DIR}/../test/${test}.cpp")
    target_link_libraries(module_${test}
            PRIVATE
            boost_decimal_module
            Boost::core
    )
    set_target_properties(module_${test} PROPERTIES CXX_SCAN_FOR_MODULES ON CXX_MODULE_STD ON)
    add_test(NAME module_${test} COMMAND module_${test})
    add_dependencies(tests module_${test})
endforeach()
