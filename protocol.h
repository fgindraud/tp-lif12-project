#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* Global unit for messages */
typedef uint32_t wireworld_message_t;
#define M_BIT_SIZE 32

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

/* cell data format in uint32_t bits :
 * |  |  |  |  | ... |  |  |  |  |  |  |
 * | C15 | C14 | ... |  C2 |  C1 |  C0 |
 */

/********************************
 * Control (from gui to server) *
 *******************************/

/* Init message
 * Followed by :
 *		xsize : 1
 *		ysize : 1
 *		data (row by row) : xsize * ysize * C_BIT_SIZE / M_BIT_SIZE + 1
 */
#define R_INIT 0u

/* Messages with no arguments */
#define R_START 1u
#define R_PAUSE 2u
#define R_STEP 3u
#define R_STOP 4u

/*******************************
 * Answer (from server to gui) *
 ******************************/

/* Show a new state of the map
 * Followed by :
 *		data (row by row) : xsize * ysize * C_BIT_SIZE / M_BIT_SIZE + 1
 */

#endif

