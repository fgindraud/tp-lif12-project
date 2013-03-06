#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

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

/*
 *
 */
int connectionSendFullUpdate (int connSock,
		char * charMap, uint32_t width, uint32_t height,
		uint32_t localXStart, uint32_t localYStart, uint32_t localXEnd, uint32_t localYEnd);

/*
 *
 */
int connectionWaitFrameRequest (int connSock);

/* Functions - connection - advanced */

int connectionSendRectUpdate (int connSock,
		char * charMap, uint32_t width, uint32_t height,
		uint32_t localXStart, uint32_t localYStart, uint32_t localXEnd, uint32_t localYEnd,
		uint32_t realXStart, uint32_t realYStart);

int connectionSendFrameEnd (int connSock);

#endif

