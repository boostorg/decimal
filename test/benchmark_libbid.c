// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <float.h>
#include <fenv.h>

typedef uint32_t Decimal32;
typedef uint64_t Decimal64;
#include "../LIBRARY/src/bid_conf.h"
#include "../LIBRARY/src/bid_functions.h"
typedef BID_UINT128 Decimal128;

#define K 20000000
#define N 5

uint32_t flag = 0;

uint32_t random_uint32(void) 
{
    uint32_t r = 0;
    for (int i = 0; i < 2; i++) 
    {
        r = (r << 16) | (rand() & 0xFFFF);
    }
    
    return r;
}

uint64_t random_uint64(void) 
{
    uint64_t r = 0;
    for (int i = 0; i < 4; i++) 
    {
        r = (r << 16) | (rand() & 0xFFFF);
    }

    return r;
}

__attribute__ ((noinline)) void generate_vector_32(Decimal32* buffer, size_t buffer_len)
{
    for (size_t i = 0; i < buffer_len; ++i)
    {
        buffer[i] = bid32_from_uint32(random_uint32(), BID_ROUNDING_TO_NEAREST, &flag);
    }
}

__attribute__ ((noinline)) void test_comparisons_32(Decimal32* data, const char* label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal32 val1 = data[k];
            Decimal32 val2 = data[k + 1];

            s += (size_t)bid32_quiet_less(val1, val2, &flag);
            s += (size_t)bid32_quiet_less_equal(val1, val2, &flag);
            s += (size_t)bid32_quiet_greater(val1, val2, &flag);
            s += (size_t)bid32_quiet_greater_equal(val1, val2, &flag);
            s += (size_t)bid32_quiet_equal(val1, val2, &flag);
            s += (size_t)bid32_quiet_not_equal(val1, val2, &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("Comparisons    <%-10s >: %-10" PRIu64 " us (s=%zu)\n", label, elapsed_time_us, s);
}

__attribute__ ((noinline)) void generate_vector_64(Decimal64* buffer, size_t buffer_len)
{
    for (size_t i = 0; i < buffer_len; ++i)
    {
        buffer[i] = bid64_from_uint64(random_uint64(), BID_ROUNDING_TO_NEAREST, &flag);
    }
}

__attribute__ ((noinline)) void test_comparisons_64(Decimal64* data, const char* label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal64 val1 = data[k];
            Decimal64 val2 = data[k + 1];

            s += (size_t)bid64_quiet_less(val1, val2, &flag);
            s += (size_t)bid64_quiet_less_equal(val1, val2, &flag);
            s += (size_t)bid64_quiet_greater(val1, val2, &flag);
            s += (size_t)bid64_quiet_greater_equal(val1, val2, &flag);
            s += (size_t)bid64_quiet_equal(val1, val2, &flag);
            s += (size_t)bid64_quiet_not_equal(val1, val2, &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("Comparisons    <%-10s >: %-10" PRIu64 " us (s=%zu)\n", label, elapsed_time_us, s);
}


__attribute__ ((__noinline__)) void generate_vector_128(Decimal128* buffer, size_t buffer_len)
{
    size_t i = 0;
    while (i < buffer_len)
    {
        buffer[i] = bid128_from_uint64(random_uint64());
        ++i;
    }
}

__attribute__ ((__noinline__)) void test_comparisons_128(Decimal128* data, const char* label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal128 val1 = data[k];
            Decimal128 val2 = data[k + 1];

            s += (size_t)bid128_quiet_less(val1, val2, &flag);
            s += (size_t)bid128_quiet_less_equal(val1, val2, &flag);
            s += (size_t)bid128_quiet_greater(val1, val2, &flag);
            s += (size_t)bid128_quiet_greater_equal(val1, val2, &flag);
            s += (size_t)bid128_quiet_equal(val1, val2, &flag);
            s += (size_t)bid128_quiet_not_equal(val1, val2, &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("Comparisons    <%-10s >: %-10" PRIu64 " us (s=%zu)\n", label, elapsed_time_us, s);
}


typedef Decimal32 (*operation_32)(Decimal32, Decimal32);

__attribute__ ((noinline)) Decimal32 add_32(Decimal32 a, Decimal32 b)
{
    return bid32_add(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}
__attribute__ ((noinline)) Decimal32 sub_32(Decimal32 a, Decimal32 b)
{
    return bid32_sub(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) Decimal32 mul_32(Decimal32 a, Decimal32 b)
{
    return bid32_mul(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) Decimal32 div_32(Decimal32 a, Decimal32 b)
{
    return bid32_div(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) void test_two_element_operation_32(Decimal32* data, operation_32 op, const char* label, const char* op_label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal32 val1 = data[k];
            Decimal32 val2 = data[k + 1];

            s += (size_t)bid32_to_int32_int(op(val1, val2), &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("%-15s<%-10s >: %-10" PRIu64 " us (s=%zu)\n", op_label, label, elapsed_time_us, s);
}

typedef Decimal64 (*operation_64)(Decimal64, Decimal64);

__attribute__ ((noinline)) Decimal64 add_64(Decimal64 a, Decimal64 b)
{
    return bid64_add(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) Decimal64 sub_64(Decimal64 a, Decimal64 b)
{
    return bid64_sub(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) Decimal64 mul_64(Decimal64 a, Decimal64 b)
{
    return bid64_mul(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) Decimal64 div_64(Decimal64 a, Decimal64 b)
{
    return bid64_div(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((noinline)) void test_two_element_operation_64(Decimal64* data, operation_64 op, const char* label, const char* op_label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal64 val1 = data[k];
            Decimal64 val2 = data[k + 1];

            s += (size_t)bid64_to_int64_int(op(val1, val2), &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("%-15s<%-10s >: %-10" PRIu64 " us (s=%zu)\n", op_label, label, elapsed_time_us, s);
}


typedef Decimal128 (*operation_128)(Decimal128, Decimal128);

__attribute__ ((__noinline__)) Decimal128 add_128(Decimal128 a, Decimal128 b)
{
    return bid128_add(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((__noinline__)) Decimal128 sub_128(Decimal128 a, Decimal128 b)
{
    return bid128_sub(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((__noinline__)) Decimal128 mul_128(Decimal128 a, Decimal128 b)
{
    return bid128_mul(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((__noinline__)) Decimal128 div_128(Decimal128 a, Decimal128 b)
{
    return bid128_div(a, b, BID_ROUNDING_TO_NEAREST, &flag);
}

__attribute__ ((__noinline__)) void test_two_element_operation_128(Decimal128* data, operation_128 op, const char* label, const char* op_label)
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    size_t s = 0;

    for (size_t n = 0; n < N; ++n)
    {
        for (size_t k = 0; k < K - 1; ++k)
        {
            Decimal128 val1 = data[k];
            Decimal128 val2 = data[k + 1];

            s += (size_t)bid128_to_int64_int(op(val1, val2), &flag);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);

    uint64_t elapsed_time_us = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_nsec - t1.tv_nsec) / 1000);
    printf("%-15s<%-10s >: %-10" PRIu64 " us (s=%zu)\n", op_label, label, elapsed_time_us, s);
}


int main()
{
    // One time init of random number generator
    srand(time(NULL));

    fedisableexcept(FE_ALL_EXCEPT);
    
    Decimal32* d32_array = malloc(K * sizeof(Decimal32));
    Decimal64* d64_array = malloc(K * sizeof(Decimal64));
    Decimal128* d128_array = malloc(K * sizeof(Decimal128));
    
    if (d32_array == NULL|| d64_array == NULL  || d128_array == NULL)
    {
        return 1;
    }

    printf("== Generating Arrays ==\n");

    generate_vector_32(d32_array, K);
    generate_vector_64(d64_array, K);
    generate_vector_128(d128_array, K);

    printf("===== Comparisons =====\n");

    test_comparisons_32(d32_array, "Decimal32");
    test_comparisons_64(d64_array, "Decimal64");
    test_comparisons_128(d128_array, "Decimal128");
    
    printf("\n===== Addition =====\n");

    test_two_element_operation_32(d32_array, add_32, "Decimal32", "Addition");
    test_two_element_operation_64(d64_array, add_64, "Decimal64", "Addition");
    test_two_element_operation_128(d128_array, add_128, "Decimal128", "Addition");

    printf("\n===== Subtraction =====\n");

    test_two_element_operation_32(d32_array, sub_32, "Decimal32", "Subtraction");
    test_two_element_operation_64(d64_array, sub_64, "Decimal64", "Subtraction");
    test_two_element_operation_128(d128_array, sub_128, "Decimal128", "Subtraction");

    printf("\n===== Multiplication =====\n");

    test_two_element_operation_32(d32_array, mul_32, "Decimal32", "Multiplication");
    test_two_element_operation_64(d64_array, mul_64, "Decimal64", "Multiplication");
    test_two_element_operation_128(d128_array, mul_128, "Decimal128", "Multiplication");

    printf("\n===== Division =====\n");

    test_two_element_operation_32(d32_array, div_32, "Decimal32", "Division");
    test_two_element_operation_64(d64_array, div_64, "Decimal64", "Division");
    test_two_element_operation_128(d128_array, div_128, "Decimal128", "Division");

    free(d32_array);
    free(d64_array);
    free(d128_array);

    return 0;
}
