from base import config, input, c_uint8, c_float  # type: ignore


@config(name='a_param', group='my_config', header="!MYC")
class MyConfig:
    an_integer_field: c_uint8
    a_float_field: c_float


@input(name='an_input', callback='my_cb', header="!MYI")
class MyInput:
    an_integer_field: c_uint8
    a_float_field: c_float
