# decimal_printer.py

import lldb

def decimal32_summary(valobj, internal_dict):
    """
    Custom summary for decimal32_t type
    Displays in scientific notation with cohort preservation
    """

    try:
        val = valobj.GetNonSyntheticValue()
        bits = val.GetChildMemberWithName("bits_").GetValueAsUnsigned()

        # TODO(mborland): Handle all the non-finite values first

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
        sign = bits & 2147483648 != 0

        if significand == 0:
            result = "0.0e+0"
        else:
            n_digits = len(str(abs(significand)))
            normalized = significand / (10 ** (n_digits - 1))
            total_exp = exp + n_digits - 1
            result = f"{'-' if not sign else ''}{normalized}e{total_exp:+}"

        return result
    except Exception as e:
        return f"<invalid decimal32_t: {e}>"

def __lldb_init_module(debugger, internal_dict):
    decimal32_pattern = r"^(const )?(boost::decimal::decimal32_t|(\w+::)*decimal32_t)( &| \*)?$"

    debugger.HandleCommand(
        f'type summary add -x "{decimal32_pattern}" -e -F decimal_printer.decimal32_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{decimal32_pattern}" -l decimal_printer.DecimalSyntheticProvider'
    )

    print("decimal32_t printer loaded successfully")

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
