# This is an extension of the example provided by bitcraze:
# - added raw format
# - added color
# - added pipe as transport

import fcntl
import os
import socket
import struct
import threading
from datetime import datetime as dt
from typing import Iterator, List, Tuple, Any, Optional
import signal
import time

import cv2
import numpy as np

# Note For best match with hardware and network realities,
# the value of bufsize should be a relatively small power of 2, for example, 4096.
# SOCK_REC_SIZE = 256
SOCK_REC_SIZE = 1024
F_SETPIPE_SZ = 1031


class RepeatTimer(threading.Timer):

    def run(self) -> None:
        while not self.finished.wait(self.interval):  # type: ignore
            self.function(*self.args, **self.kwargs)  # type: ignore


class Client:
    color: int
    encoding: str = ""
    shape: Tuple[int, int] = (0, 0)
    fps: float = 0.0
    frame: np.ndarray
    ts: List[dt]
    receive: Iterator[bytes]
    ok: bool
    number_of_frames: int = 0
    timer: Optional[RepeatTimer]
    connected: bool = False

    @property
    def size(self) -> int:
        return self.shape[0] * self.shape[1]

    def run(self) -> Iterator[np.ndarray]:
        has_received_header = False
        has_received_shape = False
        self.encoding = ""
        bs = b''
        self.color = 0
        for rx in self.receive:
            bs += rx
            if not has_received_header:
                start_raw = bs.find(b"?IMG")
                start_jpeg = bs.find(b"\xff\xd8")
                if start_raw >= 0 and (start_jpeg < 0 or start_jpeg > start_raw):
                    self.encoding = "raw"
                    bs = bs[start_raw + 4:]
                    buffer = b''
                    has_received_header = True
                    has_received_shape = False
                if start_jpeg >= 0 and (start_raw < 0 or start_raw > start_jpeg):
                    self.encoding = "jpeg"
                    has_received_header = True
                    bs = bs[start_jpeg:]
                    buffer = b''
                if not has_received_header:
                    bs = b''
                    continue
            if self.encoding == 'raw':
                if has_received_header and not has_received_shape:
                    buffer += bs
                    if len(buffer) >= 6:
                        w, h, self.color = struct.unpack('HHH', buffer[:6])
                        self.shape = (h, w)
                        buffer = buffer[6:]
                        has_received_shape = True
                        bs = b''
                elif has_received_shape:
                    end = bs.find(b"!IMG")
                    if end >= 0:
                        buffer += bs[0: end]
                        bs = bs[end:]
                        if(len(buffer) == self.size):
                            frame = np.frombuffer(buffer, dtype=np.uint8)
                            frame = frame.reshape(self.shape)
                            if self.color == 1:
                                frame = cv2.cvtColor(frame, cv2.COLOR_BAYER_BG2BGR)
                            elif self.color == 2:
                                frame = cv2.cvtColor(frame, cv2.COLOR_BAYER_GB2BGR)
                            elif self.color == 3:
                                frame = cv2.cvtColor(frame, cv2.COLOR_BAYER_GR2BGR)
                            elif self.color == 4:
                                frame = cv2.cvtColor(frame, cv2.COLOR_BAYER_RG2BGR)
                            self.ts.append(dt.now())
                            self.number_of_frames += 1
                            yield frame
                        else:
                            print(f"Expected {self.size} pixels ... got {len(buffer)}")
                            print(buffer[:8])
                        has_received_header = False
                        has_received_shape = False
                    else:
                        buffer += bs
                        bs = b''
            else:
                if has_received_header:
                    end = bs.find(b"\xff\xd9")
                    if end >= 0:
                        buffer += bs[0: end]
                        # print('buffer len', len(buffer))
                        data = np.frombuffer(buffer, dtype=np.uint8)
                        frame = cv2.imdecode(data, cv2.IMREAD_GRAYSCALE)
                        if frame is not None:
                            self.shape = frame.shape
                            self.ts.append(dt.now())
                            self.number_of_frames += 1
                            yield frame
                        bs = bs[end:]
                        has_received_header = False
                    else:
                        buffer += bs
                        bs = b''
        self._stop()

    def update_fps(self) -> None:
        if not self.ok or not self.connected:
            return
        if(len(self.ts) == 0):
            self.fps = 0.0
        elif len(self.ts) == 1:
            self.fps = 1.0
        else:
            self.fps = (len(self.ts) - 1) / (self.ts[-1] - self.ts[0]).total_seconds()
        self.ts = self.ts[-2:]
        if self.fps or self.shape:
            print(f'\r{self.shape[1]} x {self.shape[0]} {self.encoding} ({self.color}) @'
                  f'{self.fps:.1f} fps, total {self.number_of_frames}', end='')

    def read_socket(self, host: str, port: int = 5000) -> Iterator[bytes]:
        while self.ok:
            self.connected = False
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as c:
                print(f"Connecting to socket on {host}:{port}...")
                # c.connect((host, port))
                c.settimeout(5)
                try:
                    c.connect((host, port))
                except (socket.timeout, ConnectionRefusedError):
                    print("Retry")
                    continue
                except OSError:
                    time.sleep(5)
                    print("Retry after 5 seconds")
                    continue
                print("Socket connected, ready to get images")
                first = True
                while self.ok:
                    try:
                        yield c.recv(SOCK_REC_SIZE)
                        if first:
                            self.number_of_frames = 0
                            print("Getting images")
                            first = False
                            self.connected = True
                    except socket.timeout:
                        break
                print("Close socket")

    def read_pipe(self, path: str) -> Iterator[bytes]:
        print(f"Connecting to pipe at {path} ...")
        self.connected = False
        if not os.path.exists(path):
            os.mkfifo(path)
        with open(path, 'rb') as c:
            fcntl.fcntl(c, F_SETPIPE_SZ, 1000000)
            print(f"Connected to pipe at {path}, ready to get images")
            first = True
            while 1:
                # os.read(fd, n)
                # Read at most n bytes from file descriptor fd.
                yield c.read(SOCK_REC_SIZE)
                if first:
                    print("Getting images")
                    first = False
                    self.connected = True

    def exit_gracefully(self, signum: Any, frame: Any) -> None:
        self.ok = False

    def __init__(self, pipe: str = '', host: str = '', port: int = 5000,
                 keep_track_of_fps: bool = True) -> None:
        self.fps = 0.0
        self.ts = []
        self.ok = True
        self.number_of_frames = 0
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)
        if keep_track_of_fps:
            self.timer = RepeatTimer(1.0, self.update_fps)
            self.timer.start()
        else:
            self.timer = None
        if pipe:
            self.receive = self.read_pipe(pipe)
        elif host:
            self.receive = self.read_socket(host, port)
        else:
            self.receive = iter([])
            print("No source")

    def _stop(self) -> None:
        if self.timer:
            self.timer.cancel()
        print("\nLoop stopped\n")
