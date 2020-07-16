#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Author: Jerome Guzzi
# Licence: MIT
#
# This script generate C code for GAP and Crazyflie that implements an UART protocol based on
# the given specifications. Call it like
# python3 generate.py <path_to_the_speficication>
# Some specifications are provided as examples.

import argparse
import ctypes
import importlib.util
import os
from datetime import datetime as dt
from typing import Tuple, Type

import jinja2

from base import Base, c_str, to_c, to_cf, to_format
from ctypes import c_float


def _f(item: Tuple[str, Type]) -> str:
    name, t = item
    return f'{name}={to_format(t)}'


def c_value(item: Tuple[str, Type]) -> str:
    if item[1] == c_float:
        prefix = '(double)'
    else:
        prefix = ''
    return f'{prefix}value->{item[0]}'


def input_struct(t: Base) -> str:
    return f'{{ .header = "{c_str(t.header_value)}", .callback = {t.cb}, .size = sizeof({t.typename}) }}'


def is_config(t: Base) -> bool:
    return hasattr(t, 'is_config')


def main(config_path: str) -> None:

    config_spec = importlib.util.spec_from_file_location("c", config_path)
    c = importlib.util.module_from_spec(config_spec)
    if not config_spec.loader:
        raise RuntimeError(f"Could not load config from {config_path}")
    config_spec.loader.exec_module(c)  # type: ignore
    configurations = [t for _, t in vars(c).items() if hasattr(t, 'is_config')]
    inputs = [t for t in vars(c).values() if hasattr(t, 'is_input')]
    hs = [len(i.header_value) for i in configurations + inputs]
    if hs and not all(hs[0] == h for h in hs[1:]):
        print("Header length should be the same for all items")
        for i in configurations + inputs:
            print(f'{i.name}: {i.header_value} ({len(i.header_value)}')
        return
    header_length = hs[0] if hs else 0
    if configurations or inputs:
        buffer_length = max(ctypes.sizeof(i) for i in configurations + inputs)
    else:
        buffer_length = 0
    template_folder = os.path.join(os.path.dirname(__file__), 'templates')
    e = jinja2.Environment(loader=jinja2.FileSystemLoader(template_folder))
    e.filters['c'] = to_c
    e.filters['cf'] = to_cf
    e.filters['f'] = _f
    e.filters['c_str'] = c_str
    e.filters['c_value'] = c_value
    e.filters['input'] = input_struct
    e.tests['config'] = is_config
    with open(config_path, 'r') as f:
        source = f.read()
    date = dt.now().strftime("%D")
    build_folder = os.path.join(os.path.dirname(__file__), 'build')
    os.makedirs(build_folder, exist_ok=True)
    cf_build_folder = os.path.join(build_folder, 'crazyflie')
    os.makedirs(cf_build_folder, exist_ok=True)
    gap_build_folder = os.path.join(build_folder, 'gap')
    os.makedirs(gap_build_folder, exist_ok=True)
    template = e.get_template('crazyflie.aideck_p.h.j2')
    with open(os.path.join(cf_build_folder, 'aideck_protocol.c'), 'w') as f:
        f.write(template.render(
            configs=configurations, inputs=inputs,
            header_length=header_length, buffer_length=buffer_length,
            source=source, date=date))
    template = e.get_template('gap.protocol.h.j2')
    with open(os.path.join(gap_build_folder, 'uart_protocol.h'), 'w') as f:
        f.write(template.render(
            configs=configurations, inputs=inputs,
            header_length=header_length, buffer_length=buffer_length,
            source=source, date=date))
    template = e.get_template('gap._protocol.c.j2')
    with open(os.path.join(gap_build_folder, 'uart_protocol.c'), 'w') as f:
        f.write(template.render(
            configs=configurations, inputs=inputs,
            header_length=header_length, buffer_length=buffer_length,
            source=source, date=date))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("path", help="the configuration file path")
    args = parser.parse_args()
    main(args.path)
