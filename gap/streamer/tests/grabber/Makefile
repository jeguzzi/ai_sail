APP = test

SRC_DIR = $(CURDIR)/../..
# ------ TEST GRABBING
APP_SRCS += test_grab_lines.c $(SRC_DIR)/himax_utils.c
APP_CFLAGS += -O3 -g -I$(SRC_DIR)
# ------ TEST GRABBING

RUNNER_CONFIG = $(SRC_DIR)/config.ini
include $(RULES_DIR)/pmsis_rules.mk
