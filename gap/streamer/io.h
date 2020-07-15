/**
 * This header provides an interfaces to open and close a fifo [named pipe] on the host FS.
 * The pipe does implement the transport protocol (pi_transport_api_t) used by the frame buffer.
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#ifndef __IO_H__
#define __IO_H__

#include "pmsis.h"

/**
 * Open the [sigleton] named pipe
 * @param  path The path on the host FS.
 * @return      The pipe or NULL in case of failure
 */
struct pi_device * open_pipe(char *path);

/**
 * Close the named pipe.
 */
void close_pipe();

#endif //__IO_H__
