#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Author: Jerome Guzzi
# Licence: MIT
# This is an extension of the [example provided by Bitcraze]
# (https://github.com/bitcraze/AIdeck_examples/blob/master/NINA/viewer.py),
# with the following additions
# - raw encoding
# - color
# - named pipe as transport
# - ability to reconnect to socket
# - statistics about recevied frames
# and using opencv instead of gtk to display the image

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
