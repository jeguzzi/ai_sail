APP = test

SRC_DIR = $(CURDIR)/../..
# ------ TEST CAMERA LOOP
<<<<<<< HEAD
APP_SRCS += test_loops.c $(SRC_DIR)/himax_utils.c $(SRC_DIR)/camera.c $(SRC_DIR)/frame_streamer_utils.c $(SRC_DIR)/nina.c
APP_CFLAGS += -O3 -g -I$(SRC_DIR)
=======
APP_SRCS += test_loops.c $(CURDIR)/../../himax_utils.c $(CURDIR)/../../camera.c $(CURDIR)/../../frame_streamer_utils.c $(CURDIR)/../../nina.c
APP_CFLAGS += -O3 -g
>>>>>>> f1d4400fa10e09bf2097dd0dc01460e902431705
APP_LDFLAGS += -lgaptools -lgaplib
# ------ TEST CAMERA LOOP

RUNNER_CONFIG = $(SRC_DIR)/config.ini
include $(RULES_DIR)/pmsis_rules.mk
