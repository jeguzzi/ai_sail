/*
 */


#ifndef __IO_H__
#define __IO_H__


// int init_fs();
// void close_fs();
struct pi_device * open_pipe(char *path);
void close_pipe();

// int PushImageToFifo(void *fifo, unsigned int W, unsigned int H, unsigned char *OutBuffer);
// void read_file(char *path, unsigned char *line, unsigned int length);

#endif //__IO_H__
