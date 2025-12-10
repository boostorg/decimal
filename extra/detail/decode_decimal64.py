# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

def decode_decimal64(bits):
    sign = bits & 9223372036854775808 != 0
    isnan = False

    if bits & 8646911284551352320 == 8646911284551352320:

        if bits & 9079256848778919936 == 9079256848778919936:
            result = "-SNAN" if sign else "SNAN"
            isnan = True
        elif bits & 8935141660703064064 == 8935141660703064064:
            result = "-SNAN" if sign else "QNAN"
            isnan = True
        elif bits & 8935141660703064064 == 8646911284551352320:
            result = "-INF" if sign else "INF"
        else:
            raise ValueError("Unknown Finite Value")

        if isnan:
            payload = bits & 9007199254740991
            if payload > 0:
                result += '(' + str(payload) + ')'

    else:
        # See decimal64_t::to_components()
        if bits & 6917529027641081856 == 6917529027641081856:
            implied_bit = 9007199254740992
            significand = implied_bit | (bits & 2251799813685247)
            exp = (bits & 2303591209400008704) >> 51
        else:
            significand = bits & 9007199254740991
            exp = (bits & 9214364837600034816) >> 53

        exp -= 398 # Bias Value

        if significand == 0:
            result = "0.0e+0"
        else:
            n_digits = len(str(abs(significand)))

            # If there is no fractional component we want to remove the decimal point and zero
            if n_digits > 1:
                normalized = significand / (10 ** (n_digits - 1))
                total_exp = exp + n_digits - 1
            else:
                normalized = significand
                total_exp = exp
            result = f"{'-' if sign else ''}{normalized:.{n_digits - 1}f}e{total_exp:+}"

    return result
