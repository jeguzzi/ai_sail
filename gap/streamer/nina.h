#ifndef __NINA_H__
#define __NINA_H__

// void wake_up_nina();
// void put_nina_to_sleep();
void init_nina(int active);
void close_nina();
struct pi_device * open_wifi();

#endif // __NINA_H__
