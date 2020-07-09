from base import Config, c_uint8, c_uint16  # type: ignore

header_length = 4


class Camera(Config):
    marginTop: c_uint16
    marginRight: c_uint16
    marginBottom: c_uint16
    marginLeft: c_uint16
    format: c_uint8  # noqa
    step: c_uint8
    target_value: c_uint8


class Stream(Config):
    on: c_uint8
    format: c_uint8  # noqa
    transport: c_uint8


configs = [
    Camera(name='camera', group='aideck_HIMAX', header="!CAM"),
    Stream(name='stream', group='aideck_stream', header="!STR"),
]

inputs = []
