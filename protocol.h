#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Data format
typedef uint8_t data_block_t;

#define C_BIT_SIZE 2

#define C_INSULATOR 0u
#define C_WIRE 1u
#define C_HEAD 2u
#define C_TAIL 3u

/* data_block_t bits : |  |  |  |  |  |  |  |  |
 *                     |  C0 |  C1 |  C2 |  C3 |
 */


// Control format
typedef uint32_t control_message_t;

/* Init message
 * Followed by :
 *		xsize : 1
 *		ysize : 1
 *		data (row by row) : xsize * ysize * C_BIT_SIZE / 32
 */
#define M_INIT 0u

#define M_START 1u
#define M_PAUSE 2u
#define M_STEP 3u

#endif

