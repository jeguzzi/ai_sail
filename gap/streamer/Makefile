APP = streamer

APP_SRCS += main.c himax_utils.c uart.c camera.c led.c nina.c io.c frame_streamer_utils.c
APP_CFLAGS += -O3 -g -DUART_COMM
APP_LDFLAGS += -lgaptools -lgaplib

RUNNER_CONFIG = $(CURDIR)/config.ini

include $(RULES_DIR)/pmsis_rules.mk
