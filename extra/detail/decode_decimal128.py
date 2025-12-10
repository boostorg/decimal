# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

from generate_string import generate_string

def decode_decimal128(bits):

    bits_high = bits >> 64
    d128_not_11_significand_mask = (562949953421311 << 64) | 18446744073709551615
    d128_11_significand_mask = (140737488355327 << 64) | 18446744073709551615

    sign = bits_high & 9223372036854775808 != 0
    isnan = False

    if bits_high & 8646911284551352320 == 8646911284551352320:

        if bits_high & 9079256848778919936 == 9079256848778919936:
            result = "-SNAN" if sign else "SNAN"
            isnan = True
        elif bits_high & 8935141660703064064 == 8935141660703064064:
            result = "-SNAN" if sign else "QNAN"
            isnan = True
        elif bits_high & 8935141660703064064 == 8646911284551352320:
            result = "-INF" if sign else "INF"
        else:
            raise ValueError("Unknown Finite Value")

        if isnan:
            payload = bits & d128_not_11_significand_mask
            if payload > 0:
                result += '(' + str(payload) + ')'

    else:
        # See decimal128_t::to_components()
        if bits_high & 6917529027641081856 == 6917529027641081856:
            implied_bit = 562949953421312 << 64
            significand = implied_bit | (bits & d128_11_significand_mask)
            exp = (bits_high & 2305702271725338624) >> 47
        else:
            significand = bits & d128_not_11_significand_mask
            exp = (bits_high & 9222809086901354496) >> 49

        exp -= 6176 # Bias Value

        result = generate_string(significand, exp, sign)

    return result
