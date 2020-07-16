#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "pmsis.h"
#include "bsp/camera.h"

#include "himax_utils.h"
#include "logging.h"

static struct pi_device camera;
static int volatile number_of_frames = 0;

#define NUMBER 10
#define PERF

// #define STARTSTOP
#define FPS 82
#define FORMAT FULL

#if FORMAT==QVGA
#define LINE_LENGTH 324
#define NUMBER_OF_LINES 244
#endif
#if FORMAT==QQVGA
#define LINE_LENGTH 162
#define NUMBER_OF_LINES 122
#endif
#if FORMAT==FULL
#define LINE_LENGTH 324
#define NUMBER_OF_LINES 324
#endif
#if FORMAT==HALF
#define LINE_LENGTH 162
#define NUMBER_OF_LINES 162
#endif

#define LINES 2
#define J (LINE_LENGTH / 2)
static int lines[LINES] = {1, NUMBER_OF_LINES-1};
// static int lines[LINES] = {1, J, NUMBER_OF_LINES-1};
// static int lines[LINES] = {1, NUMBER_OF_LINES-1};
static int index[LINES];
static pi_task_t task[LINES];
static PI_L2 unsigned char buffer[NUMBER_OF_LINES * LINE_LENGTH];

#ifdef PERF
static unsigned int cycles[NUMBER][LINES];
#endif

static void end_of_frame(void *arg) {
  if(number_of_frames >= NUMBER) return;
  int i = *(int *)arg;
  cycles[number_of_frames][i] = pi_perf_read(PI_PERF_CYCLES);
  int ni = (i + 2) % LINES;
  int number_of_lines;
  if(ni >= 1)
  {
    number_of_lines = lines[ni] - lines[ni-1];
  }
  else{
#ifdef STARTSTOP
    number_of_lines = lines[0];
#else
    number_of_lines = lines[0] - lines[LINES-1] + NUMBER_OF_LINES;
#endif
  }
  // printf("%d %d %d %d\n", i, ni, number_of_lines, number_of_frames);
  if(i == LINES - 1)
  {
    number_of_frames++;
#ifdef STARTSTOP
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
#else
    // if(number_of_frames < 2)
    // {
    //   pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    //   pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    // }
#endif
  }
  pi_camera_capture_async(&camera, buffer, LINE_LENGTH * number_of_lines, pi_task_callback(task+ni, end_of_frame, index+ni));
}

int main()
{
  pi_freq_set(PI_FREQ_DOMAIN_FC, 100000000); // 1000 Mhz
  if(himax_open(&camera))
  {
    LOG_ERROR("Failed to open camera\n");
    return -1;
  }

  himax_configure(&camera, 0);
  himax_enable_ae(&camera, 1);
  himax_set_format(&camera, FORMAT);
  himax_set_fps(&camera, FPS, FORMAT);

  for (size_t i = 0; i < LINES; i++) {
    index[i] = i;
  }

  printf("-------- Start Loop: record time at %d lines: ", LINES);
  for (size_t i = 0; i < LINES; i++) {
    printf("%d ", lines[i]);
  }
  printf("--------\n");
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
#ifdef PERF
  pi_perf_conf(1<<PI_PERF_CYCLES | 1<<PI_PERF_ACTIVE_CYCLES);
  pi_perf_start();
  pi_perf_reset();
  int start = pi_perf_read(PI_PERF_CYCLES);
#endif
  pi_camera_capture_async(&camera, buffer, LINE_LENGTH * lines[0], pi_task_callback(task, end_of_frame, index));
#if LINES > 1
  pi_camera_capture_async(&camera, buffer, LINE_LENGTH * (lines[1] - lines[0]), pi_task_callback(task+1, end_of_frame, index+1));
#endif
  while(number_of_frames < NUMBER)
  {
    pi_yield();
  }
  printf("------------------------------------------------------------------------\n");
#ifdef PERF
  int end = pi_perf_read(PI_PERF_CYCLES);
  printf("Acquired %d frames in %d ms\n", number_of_frames, (end - start) / 100000);
  int *cs = (int *)cycles;
  printf("ts=[%d,", cs[0] / 100);
  for (size_t i = 1; i < number_of_frames * LINES; i++) {
    printf("%d,", (cs[i] - cs[i-1]) / 100);  //us
  }
  printf("]\n");
#endif
  pi_camera_close(&camera);
  return 0;
}
