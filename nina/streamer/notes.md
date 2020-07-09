MAC Address: A4:CF:12:84:2A:FC
host name espressif

sleeping
======

see https://github.com/u-blox/nina-w10-power-test
and https://github.com/espressif/esp-idf/tree/release/v3.3/examples/system/deep_sleep

gpio
====

https://wiki.bitcraze.io/projects:crazyflie2:expansionboards:aideck

button = sysboot = gpio_27 = touch1 = #0


see also

https://github.com/espressif/esp-idf/blob/release/v3.3/components/driver/test/test_gpio.c


I cannot use (??) the button as an independent GPIO (to change state) because it triggers rebooting of the NINA.

Should I try with uart?


docs
====

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html

uart2 Cf

USART_InitStructure.USART_BaudRate            = baudrate;
USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
USART_InitStructure.USART_StopBits            = USART_StopBits_1;
USART_InitStructure.USART_Parity              = USART_Parity_No ;
USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;


#define ESP_OK          0
#define ESP_FAIL        -1    



non so perché ma dopo un po' (qualche ora di test gap non parte più, già successo ieri e basta che si aspetta un po')


NINA:

sono capace a
- mandare e ricevere via UART (mantenendo i LOG attivi), cambiando il led a seconda del messaggio (originato da cfclient)
- attivare / disattivare wifi (e cambiare da AP a managed, almeno a built time)
- abilitare deep sleep (e svegliarlo con sysboot)

sarebbe bello:
- abilitare light sleep e svegliarlo da uart (sempre via cfclient)
- usare uart per attivare/disattivare wifi e cambiare stream da jpeg a raw.
- stream raw (modificando il codice dal loro firmware) [usando lo streamer del CF a 160 x 96]
