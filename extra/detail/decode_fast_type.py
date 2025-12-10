# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

from generate_string import generate_string

def decode_decimal_fast32(significand, exp, sign):
    return generate_string(significand, exp, sign)

