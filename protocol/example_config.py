from base import Config, Input, c_uint8, c_float

header_length = 4


class MyConfig(Config):
    an_integer_field: c_uint8
    a_float_field: c_float


class MyInput(Input):
    an_integer_field: c_uint8
    a_float_field: c_float


configs = [
    MyConfig(name='my_config', group='my_config', header="!MYC")
]

inputs = [
    MyInput(name='my_input', header="\\x90\\x19\\x08\\x31", callback="my_cb")
]
