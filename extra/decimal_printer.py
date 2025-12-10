# Copyright 2025 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

from detail.decode_decimal32 import decode_decimal32
from detail.decode_decimal64 import decode_decimal64
from detail.decode_decimal128 import decode_decimal128
from detail.decode_fast_type import decode_fast_type
import lldb

def decimal32_summary(valobj, internal_dict):
    """
    Custom summary for decimal32_t type
    Displays in scientific notation with cohort preservation
    """

    try:
        val = valobj.GetNonSyntheticValue()
        bits = val.GetChildMemberWithName("bits_").GetValueAsUnsigned()
        return decode_decimal32(bits)

    except Exception as e:
        return f"<invalid decimal32_t: {e}>"

def decimal64_summary(valobj, internal_dict):
    """
    Custom summary for decimal64_t type
    Displays in scientific notation with cohort preservation
    """

    try:
        val = valobj.GetNonSyntheticValue()
        bits = val.GetChildMemberWithName("bits_").GetValueAsUnsigned()
        return decode_decimal64(bits)

    except Exception as e:
        return f"<invalid decimal64_t: {e}>"

def decimal128_summary(valobj, internal_dict):
    """
    Custom summary for decimal128_t type
    Displays in scientific notation with cohort preservation
    """

    try:
        val = valobj.GetNonSyntheticValue()
        bits = val.GetChildMemberWithName("bits_")
        bits_high = bits.GetChildMemberWithName("high").GetValueAsUnsigned()
        bits_low = bits.GetChildMemberWithName("low").GetValueAsUnsigned()
        combined_bits = (bits_high << 64) | bits_low
        return decode_decimal128(combined_bits)

    except Exception as e:
        return f"<invalid decimal128_t: {e}>"

def __lldb_init_module(debugger, internal_dict):
    decimal32_pattern = r"^(const )?(boost::decimal::decimal32_t|(\w+::)*decimal32_t)( &| \*)?$"
    decimal64_pattern = r"^(const )?(boost::decimal::decimal64_t|(\w+::)*decimal64_t)( &| \*)?$"
    decimal128_pattern = r"^(const )?(boost::decimal::decimal128_t|(\w+::)*decimal128_t)( &| \*)?$"

    debugger.HandleCommand(
        f'type summary add -x "{decimal32_pattern}" -e -F decimal_printer.decimal32_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{decimal32_pattern}" -l decimal_printer.DecimalSyntheticProvider'
    )

    print("decimal32_t printer loaded successfully")

    debugger.HandleCommand(
        f'type summary add -x "{decimal64_pattern}" -e -F decimal_printer.decimal64_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{decimal64_pattern}" -l decimal_printer.DecimalSyntheticProvider'
    )

    print("decimal64_t printer loaded successfully")

    debugger.HandleCommand(
        f'type summary add -x "{decimal128_pattern}" -e -F decimal_printer.decimal128_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{decimal128_pattern}" -l decimal_printer.DecimalSyntheticProvider'
    )

    print("decimal128_t printer loaded successfully")

class DecimalSyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def num_children(self):
        return 1

    def get_child_index(self, name):
        if name == "bits_":
            return 0
        return -1

    def get_child_at_index(self, index):
        if index == 0:
            return self.valobj.GetChildMemberWithName("bits_")
        return None

    def update(self):
        pass

    def has_children(self):
        return True

class DecimalFastSyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def num_children(self):
        return 3

    def get_child_index(self, name):
        if name == "significand_":
            return 0
        elif name == "exponent_":
            return 1
        elif name == "sign_":
            return 2
        else:
            return -1

    def get_child_at_index(self, index):
        if index == 0:
            return self.valobj.GetChildMemberWithName("significand_")
        elif index == 1:
            return self.valobj.GetChildMemberWithName("exponent_")
        elif index == 2:
            return self.valobj.GetChildMemberWithName("sign_")
        else:
            return None

    def update(self):
        pass

    def has_children(self):
        return True

