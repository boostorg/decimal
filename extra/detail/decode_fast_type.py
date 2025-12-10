# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

from generate_string import generate_string

def decode_fast_type(significand, exp, sign):
    return generate_string(significand, exp, sign)
    
