import ctypes
from ctypes import (c_char, c_float, c_int8, c_int16, c_int32, c_uint8,
                    c_uint16, c_uint32, Structure)
from typing import Optional, Type, Callable

_conversions = {
    c_float:  ('float'   , 'FLOAT' , '%.3g'),  # noqa
    c_uint8:  ('uint8_t' , 'UINT8' , '%u'  ),  # noqa
    c_uint16: ('uint16_t', 'UINT16', '%u'  ),  # noqa
    c_uint32: ('uint32_t', 'UINT32', '%u'  ),  # noqa
    c_int8:   ('int8_t'  , 'INT8'  , '%d'  ),  # noqa
    c_int16:  ('int16_t' , 'INT16' , '%d'  ),  # noqa
    c_int32:  ('int32_t' , 'INT32' , '%d'  ),  # noqa
    c_char:   ('uint8_t' , 'UINT16', '%c'  ),  # noqa
}


def to_c(t: Type) -> str:
    return _conversions[t][0]


def to_cf(t: Type) -> str:
    return _conversions[t][1]


def to_format(t: Type) -> str:
    return _conversions[t][2]

# def size(Structure):


class Base(Structure):
    value: str
    header: str
    typename: str
    name: str
    cb: str


def base(name: str, header: str, cls: Type) -> Type:
    return type(
        cls.__name__, (Base, ),
        {'_pack_': 1,
         '_fields_': [(n, t) for n, t in cls.__annotations__.items()
                      if t.__name__ in dir(ctypes)],
         'header': '__' + name + '_header',
         'cb': '__' + name + '_cb',
         'value': '__' + name + '__config',
         'typename': name + "_t",
         'header_value': header,
         'name': name})


def config(name: str, group: str, header: str) -> Callable[[Type], Type]:

    def make_config(cls: Type) -> Type:
        C = base(name=name, header=header, cls=cls)
        C.group = group
        C.is_config = True
        return C

    return make_config


def input(name: str, header: str, callback: Optional[str] = None  # noqa
          ) -> Callable[[Type], Type]:

    def make_input(cls: Type) -> Type:
        C = base(name=name, header=header, cls=cls)
        C.set_cb = callback if callback is not None else name + '_callback'
        C.is_input = True
        return C

    return make_input
