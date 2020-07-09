from ctypes import (c_char, c_float, c_int8, c_int16, c_int32, c_uint8,
                    c_uint16, c_uint32)
from typing import Optional, Type

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


class Base:
    def __init__(self, name: str, header: str, group: str = ''):
        self.name = name
        self.group = group
        self.header_value = header
        self._cb = '__' + self.name + '_cb'

    @property
    def typename(self) -> str:
        return self.name + "_t"

    @property
    def value(self) -> str:
        return '__' + self.name + '__config'

    @property
    def cb(self) -> str:
        return self._cb

    @property
    def header(self) -> str:
        return '__' + self.name + '_header'


class Input(Base):
    @property
    def set_cb(self) -> str:
        return self.name + '_callback'

    def __init__(self, name: str, header: str, group: str = '',
                 callback: Optional[str] = None):
        super(Input, self).__init__(name=name, group=group,
                                    header=header)
        if callback is not None:
            self._cb = callback


class Config(Base):
    pass
