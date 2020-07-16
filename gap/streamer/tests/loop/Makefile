APP = test

# ------ TEST CAMERA LOOP
APP_SRCS += test_loops.c $(CURDIR)/../../himax_utils.c $(CURDIR)/../../camera.c $(CURDIR)/../../frame_streamer_utils.c $(CURDIR)/../../nina.c
APP_CFLAGS += -O3 -g
APP_LDFLAGS += -lgaptools -lgaplib
# ------ TEST CAMERA LOOP

RUNNER_CONFIG = $(CURDIR)/../../config.ini
include $(RULES_DIR)/pmsis_rules.mk
