# UART Protocol generation

## Protocol

The Crazyflie and the AI-deck communicate over UART, to which we added a simple application-level protocol to exchange C structures.

The messages are composed by an `N` byte long header that identify the message type, followed by the buffer (of fixed, known size) containing the structure.

## Generation

The script `generate.py` generates the C code that implements the protocol from a specification written in Python.

```bash
python3 generate.py <path_to_the_config>
```

For example, this config file
```
from base import Config, Input, c_uint8

header_length = 4

class MyConfig(Config):
    an_integer_field: c_uint8
    a_float_field: c_float

class MyInput(Input):
    an_integer_field: c_uint8
    a_float_field: c_float

configs = [
    MyConfig(name='myconfig', group='myconfig', header="!MYC")
]

inputs = [
    MyInput(name='my_input', group='myinput', header="\\x90\\x19\\x08\\x31")
]
```
will generate the code for 1 configuration and 1 input.

We include the specification for the image streaming gap application.

The structure are of two types: configuration and inputs.

### Configuration

A configuration is group together a set of [Crazyflie firmware] parameters that are synchronized between Crazyflie and GAP. On the Crazyflie side, the driver checks at regular intervals which parameters have changed and send them to the GAP. In turns, the GAP should send the configuration each time it changes (and in particular at initialization and after each requested change).

For example, the above specification is converted to two parameters:
```
PARAM_GROUP_START(my_config)
PARAM_ADD(PARAM_UINT8, an_integer_field, &(__my_config__config.value.an_integer_field))
PARAM_ADD(PARAM_FLOAT, a_float_field, &(__my_config__config.value.a_float_field))
PARAM_GROUP_STOP(my_config)
```


### Input

An input is a structured message from the GAP to the Crazyflie. For every input, the user has to implement a callback, which get triggered every time a new message is received. For the above example, the user has to define the callback
```
static void my_input_callback(my_input_t *);
```
which is defined in the generate file.


## Building

The generated files should be included in the building tree:
- files in `build/crazyflie/` have to be copied in `<crazyflie_firmware>/src/deck/driver/src`
- files in `build/gap` in `<gap_app>/src`

The generate files are not meant to be modified or compiled as standalone file, but included in other files that implement the rest of the UART communication:
- `<crazyflie_firmware>/src/deck/driver/src/aideck.c`
- `<gap_app>/src/uart.c`

which are also provided in this repo.
