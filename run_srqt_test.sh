#!/bin/bash
# Compile and run sqrt-related functional tests

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
TEST_DIR="${PROJECT_ROOT}/test"
BUILD_DIR="${PROJECT_ROOT}/build_tests"

mkdir -p "$BUILD_DIR"

# Test cases: test_name:description
declare -a TESTS=(
    "test_sqrt:Main sqrt test"
    "test_constants:sqrt constants test (sqrt2, sqrt3, inv_sqrt, etc.)"
    "test_cmath:cmath functions test (including sqrt)"
    "github_issue_1107:GitHub issue 1107 regression test"
    "github_issue_1110:GitHub issue 1110 regression test"
)

# Compilation flags
CXX_FLAGS="-std=c++14 -O2 -I${PROJECT_ROOT}/include"

echo "=========================================="
echo "  Building sqrt functional tests"
echo "=========================================="

# Compile all tests
for test_entry in "${TESTS[@]}"; do
    test_name="${test_entry%%:*}"
    test_desc="${test_entry#*:}"
    
    echo "→ Compiling ${test_name} (${test_desc})..."
    
    if g++ ${CXX_FLAGS} "${TEST_DIR}/${test_name}.cpp" -o "${BUILD_DIR}/${test_name}"; then
        echo "  ✓ Compiled successfully"
    else
        echo "  ✗ Compilation failed"
        exit 1
    fi
done

echo ""
echo "=========================================="
echo "  Running tests"
echo "=========================================="

passed=0
failed=0

for test_entry in "${TESTS[@]}"; do
    test_name="${test_entry%%:*}"
    test_desc="${test_entry#*:}"
    
    echo "→ Running ${test_name}..."
    
    if "${BUILD_DIR}/${test_name}"; then
        echo "  ✓ PASSED"
        ((passed++))
    else
        echo "  ✗ FAILED"
        ((failed++))
    fi
    echo ""
done

echo "=========================================="
echo "  Summary"
echo "=========================================="
echo "Passed: ${passed}"
echo "Failed: ${failed}"
echo "Total:  $((passed + failed))"

if [ $failed -eq 0 ]; then
    echo ""
    echo "✓ All tests passed!"
    exit 0
else
    echo ""
    echo "✗ Some tests failed"
    exit 1
fi
