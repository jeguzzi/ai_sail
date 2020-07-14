#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This is an extension of the example provided by bitcraze:
# - added raw format
# - added color
# - added pipe as transport

import argparse
import cv2
from client import Client
import numpy as np


def display(frame: np.ndarray) -> None:
    cv2.imshow('camera stream', frame)
    cv2.waitKey(1)


def main() -> None:
    # Args for setting IP/port of AI-deck. Default settings are for when
    # AI-deck is in AP mode.
    parser = argparse.ArgumentParser(description='Connect to AI-deck streamer')
    parser.add_argument("-host", default="192.168.4.1", metavar="host", help="AI-deck host")
    parser.add_argument("-port", type=int, default='5000', metavar="port", help="AI-deck port")
    parser.add_argument("-pipe", type=str, default='', metavar="pipe", help="Pipe path")
    args = parser.parse_args()
    client = Client(host=args.host, port=args.port, pipe=args.pipe)
    for frame in client.run():
        display(frame)


if __name__ == '__main__':
    main()
