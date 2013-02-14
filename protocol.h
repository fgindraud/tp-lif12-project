#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* Global unit for messages
 * All messages will be a multiple of this size, and will be cut in pieces of this type.
 * In the message description below, the sizes are counted in wireworld_message_t.
 *
 * ---------
 * Message dummy :
 *		field : s [v]
 * ---------
 *
 *  Means that the message of type "dummy" is composed of one field, with a size of s
 *  wireworld_message_t blocks, and a specific value v.
 */
typedef uint32_t wireworld_message_t;
#define M_BIT_SIZE 32

/********************************
 * Control (from gui to server) *
 *******************************/

/* Init message (initializes the simulation with the init cell map, doesn't start) :
 *	   id    : 1 [R_INIT]
 *	   xsize : 1
 *	   ysize : 1
 *	   frame : xsize * ysize * C_BIT_SIZE / M_BIT_SIZE + 1
 */
#define R_INIT 0u

/* Start message (start or restart the simulation) :
 *	   id : 1 [R_START]
 */
#define R_START 1u

/* Pause message (temporarily halt the simulation, until a start command is received) :
 *	   id : 1 [R_PAUSE]
 */
#define R_PAUSE 2u

/* Step message (in pause mode only ; computes one step of the simulation) :
 *	   id : 1 [R_STEP]
 */
#define R_STEP 3u

/* Stop message (stops the simulation, you can close the connection) :
 *	   id : 1 [R_STOP]
 */
#define R_STOP 4u

/*******************************
 * Answer (from server to gui) *
 ******************************/

/* The server should wait for connections, then wait for init requests.
 * It will then send frames after frames to the server, without any header.
 * The server is responsible for the timing control, and should answer correctly to pause/step/resume requests.
 * It is a good practice to send a first frame (copy of the init map) after the init step ;
 * this will allow to check if the format was respected (and is cool for the eye).
 */

/* Show a new state of the map "message" :
 *	   frame : xsize * ysize * C_BIT_SIZE / M_BIT_SIZE + 1
 */

/********************
 * Cell description *
 *******************/

/* Cell constants */
#define C_INSULATOR 0u
#define C_WIRE 1u
#define C_HEAD 2u
#define C_TAIL 3u

/* Cell bit size */
#define C_BIT_SIZE 2
#define C_BIT_MASK 0x3

/* A frame is a representatition of the cell map state.
 * It the sequence of all cell states, read row by row.
 * This sequence is cut into pieces of wireworld_message_t, each containing 16
 * celle states in the given format :
 *
 * cell data format in uint32_t bits :
 * |  |  |  |  | ... |  |  |  |  |  |  |
 * | C15 | C14 | ... |  C2 |  C1 |  C0 |
 */

#endif

