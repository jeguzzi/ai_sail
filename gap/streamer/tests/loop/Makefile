APP = test

# ------ TEST CAMERA LOOP
APP_SRCS += test_loops.c himax_utils.c camera.c frame_streamer_utils.c nina.c
APP_CFLAGS += -O3 -g
APP_LDFLAGS += -lgaptools -lgaplib
# ------ TEST CAMERA LOOP

RUNNER_CONFIG = $(CURDIR)/../../config.ini
include $(RULES_DIR)/pmsis_rules.mk
