# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

from generate_string import generate_string

def decode_decimal32(bits):
    sign = bits & 2147483648 != 0
    isnan = False

    if bits & 2013265920 == 2013265920:

        if bits & 2113929216 == 2113929216:
            result = "-SNAN" if sign else "SNAN"
            isnan = True
        elif bits & 2080374784 == 2080374784:
            result = "-SNAN" if sign else "QNAN"
            isnan = True
        elif bits & 2080374784 == 2013265920:
            result = "-INF" if sign else "INF"
        else:
            raise ValueError("Unknown Finite Value")

        if isnan:
            payload = bits & 8388607
            if payload > 0:
                result += '(' + str(payload) + ')'

    else:
        # See decimal32_t::to_components()
        d32_comb_11_mask = 1610612736
        if bits & d32_comb_11_mask == d32_comb_11_mask:
            implied_bit = 8388608
            significand = implied_bit | (bits & 2097151)
            exp = (bits & 534773760) >> 21
        else:
            significand = bits & 8388607
            exp = (bits & 2139095040) >> 23

        exp -= 101 # Bias Value

        result = generate_string(significand, exp, sign)

    return result
