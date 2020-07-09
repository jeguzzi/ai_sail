import jinja2
from typing import Type
from base import Base, Config, to_c, to_cf, to_format
import os
from typing import Tuple
from datetime import datetime as dt
import importlib.util
import argparse


def _f(item: Tuple[str, Type]) -> str:
    name, t = item
    return f'{name}={to_format(t)}'


def input_struct(t: Base) -> str:
    return f'{{ .header = "{t.header_value}", .callback = {t.cb}, .size = sizeof({t.typename}) }}'


def is_config(t: Base) -> bool:
    return isinstance(t, Config)


def main(config_path: str) -> None:

    config_spec = importlib.util.spec_from_file_location("c", config_path)
    c = importlib.util.module_from_spec(config_spec)
    if not config_spec.loader:
        raise RuntimeError(f"Could not load config from {config_path}")
    config_spec.loader.exec_module(c)  # type: ignore
    e = jinja2.Environment(loader=jinja2.FileSystemLoader('./templates'))
    e.filters['c'] = to_c
    e.filters['cf'] = to_cf
    e.filters['f'] = _f
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
        f.write(template.render(config=c, source=source, date=date))
    template = e.get_template('gap.protocol.h.j2')
    with open(os.path.join(gap_build_folder, 'uart_protocol.h'), 'w') as f:
        f.write(template.render(config=c))
    template = e.get_template('gap._protocol.c.j2')
    with open(os.path.join(gap_build_folder, 'uart_protocol.c'), 'w') as f:
        f.write(template.render(config=c))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("path", help="the configuration file path")
    args = parser.parse_args()
    main(args.path)
