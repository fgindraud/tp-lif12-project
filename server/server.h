#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../protocol/protocol.h"

/* Param */
#define SERVER_BACKLOG 5

/* Functions - server */

/* Launches the server on the given port.
 * Returns -1 on error (and print an error message), the server socket (>0) on success.
 */
int serverInit (int port);

/* Wait for a connection.
 * Returns : -1 on error (+error message), a new socket (>0) on success.
 */
int serverAccept (int serverSock);

/* Functions - connection */

/* After a connection is opened, call this function to get the initial frame,
 * and other settings (width, height, and sampling parameters).
 * Returns -1 on error, 0 on success.
 *
 * *firstFrame will contain a malloc-ed array after the call, which should be freed later.
 * Access to cell at coordinates (x, y) is done using *firstFrame[x + y * (*height)]
 */
int connectionWaitForInit (int connSock,
		uint32_t * width, uint32_t * height, uint32_t * sampling, char ** firstFrame);

/* Call this function to send the new entire frame which was computed.
 * It will send the rectangle from point (localXStart, localYStart) included to point
 * (localXEnd, localYEnd) excluded, extracted from the 2d array of char 'charMap' with
 * width 'width' and height 'height'.
 * Returns -1 on error, 0 on success, 1 on connection closed.
 */
int connectionSendFullUpdate (int connSock,
		char * charMap, uint32_t width, uint32_t height,
		uint32_t localXStart, uint32_t localYStart, uint32_t localXEnd, uint32_t localYEnd);

/* Blocking function which wait for a frame request message.
 * Returns -1 on error, 0 on success, and 1 on connection closed.
 */
int connectionWaitFrameRequest (int connSock);

/* Functions - connection - advanced
 *
 * For people who want to only update parts of the image (and don't consume bandwidth for
 * constant cells). You can send a sequence of updates of rectangular regions of the global
 * buffer, from any temporary buffer which is a 2d-array of char. You must end the sequence
 * with a A_FRAME_END message, using connectionSendFrameEnd().
 */

/* These function will send a rectangle update (not necessarily the entire frame), with
 * the same argument semantics than connectionSendFullUpdate.
 * In addition we give the target coordinates of where the rectangle should be copied to,
 * in real map coordinates.
 * Returns -1 on error, 0 on success, 1 on connection closed.
 */
int connectionSendRectUpdate (int connSock,
		char * charMap, uint32_t width, uint32_t height,
		uint32_t localXStart, uint32_t localYStart, uint32_t localXEnd, uint32_t localYEnd,
		uint32_t realXStart, uint32_t realYStart);

/* This function will mark the end of the sequence of modifications of this frame.
 * Returns -1 on error, 0 on success, 1 on connection closed.
 */
int connectionSendFrameEnd (int connSock);

#endif

