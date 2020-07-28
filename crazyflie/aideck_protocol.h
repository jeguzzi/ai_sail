/*
  This file was autogenerate on 07/28/20 from the following Python config:

  from base import config, c_uint8, c_uint16  # type: ignore


  @config(name='camera', group='aideck_HIMAX', header="!CAM")
  class Camera:
      marginTop: c_uint16
      marginRight: c_uint16
      marginBottom: c_uint16
      marginLeft: c_uint16
      format: c_uint8  # noqa
      step: c_uint8
      target_value: c_uint8
      ae: c_uint8
      fps: c_uint8


  @config(name='stream', group='aideck_stream', header="!STR")
  class Stream:
      on: c_uint8
      format: c_uint8  # noqa
      transport: c_uint8

*/

#define VERBOSE_RX
#define VERBOSE_TX