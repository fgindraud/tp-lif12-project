#include "server.h"

/* Proto/macro */
void perform_simulation (int sock);
void update_map (int * dir, char ** maps, uint32_t xs, uint32_t ys);

/* Small utils */
static inline char * map (char * tab, int x, int y, int xsize) { return &tab[x + y * xsize]; }

/* main */
int main (void) {
	int serverSock = serverInit (8000);
	while (serverSock != -1) {
		res = serverAccept (sock);
		if (res > 0) {
			perform_simulation (res);
			close (res);
		} else {
			close (serverSock);
			serverSock = -1;
		}
	}
	return EXIT_SUCCESS;
}

/* Simulation */
void perform_simulation (int sock) {
	uint32_t xsize, ysize;
	char * maps [2];

	if (wait_for_init (sock, &xsize, &ysize, maps) == 0) {
		int updatedMap = 0;

		uint32_t bitMapSize = xsize * ysize * C_BIT_SIZE / M_BIT_SIZE + 1;
		wireworld_message_t * bitMap = malloc (bitMapSize * sizeof (wireworld_message_t));
		assert (bitMap != NULL);
		
		while (1) {
			// Convert and send old map.
			map_to_network (bitMap, maps[updatedMap], xsize, ysize);
			if (send_words (sock, bitMap, bitMapSize) == -1)
				break;

			// Wait 0.5s
			usleep (500000);

			// Compute new step
			update_map (&updatedMap, maps, xsize, ysize);
		}

		free (bitMap);
		free (maps[0]);
		free (maps[1]);
	}
}

/* simulation parts */
int wait_for_init (int sock, uint32_t * xs, uint32_t * ys, char ** maps) {
	wireworld_message_t message[3];
	if (read_words (sock, message, 3) == 0 && message[0] == R_INIT) {
		// If init message, retrieve sizes
		*xs = message[1];
		*ys = message[2];
		// Prepare buffer and read init map
		uint32_t count = *xs * *ys * C_BIT_SIZE / M_BIT_SIZE + 1;
		wireworld_message_t * buf = malloc (count * sizeof (wireworld_message_t));
		assert (buf != NULL);
		if (read_words (sock, buf, count) == 0) {
			// Alloc maps
			maps[0] = malloc ((*xs + 2) * (*ys + 2) * sizeof (char));
			maps[1] = malloc ((*xs + 2) * (*ys + 2) * sizeof (char));
			assert (maps[0] != NULL); assert (maps[1] != NULL);

			// Init maps borders with insulators (simplest method to handle border problems)
			uint32_t i;
			// horiz lines
			for (i = 0; i < *xs + 2; ++i) {
				*map (maps[0], i, 0, *xs + 2) = C_INSULATOR;
				*map (maps[0], i, *ys + 1, *xs + 2) = C_INSULATOR;
				*map (maps[1], i, 0, *xs + 2) = C_INSULATOR;
				*map (maps[1], i, *ys + 1, *xs + 2) = C_INSULATOR;
			}
			// vert lines
			for (i = 0; i < *ys + 2; ++i) {
				*map (maps[0], 0, i, *xs + 2) = C_INSULATOR;
				*map (maps[0], *xs + 1, i, *xs + 2) = C_INSULATOR;
				*map (maps[1], 0, i, *xs + 2) = C_INSULATOR;
				*map (maps[1], *xs + 1, i, *xs + 2) = C_INSULATOR;
			}
			// Convert map
			network_to_map (buf, maps[0], *xs, *ys);
			// destroy temp buffer
			free (buf);
			return 0;
		} else {
			fprintf (stderr, "Unable to read init map\n");
			free (buf);
			return -1;
		}
	} else {
		fprintf (stderr, "Did not received init message\n");
		return -1;
	}
}

void update_map (int * dir, char ** maps, uint32_t xs, uint32_t ys) {
	char * fromMap = maps[*dir];
	char * toMap = maps[1 - *dir];
	uint32_t i, j;
				
	static const int diffs[][2] = {
		{ -1, -1 },
		{ 0, -1 },
		{ 1, -1 },
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ -1, 1 },
		{ -1, 0 }
	};

	for (i = 1; i < xs + 1; ++i)
		for (j = 1; j < ys + 1; ++j) {
			char state = *map (fromMap, i, j, xs + 2);
			char * out = map (toMap, i, j, xs + 2);

			if (state == C_INSULATOR) {
				*out = C_INSULATOR;
			} else if (state == C_WIRE) {
				int nbHeads = 0;
				int k;
				for (k = 0; k < 8; ++k)
					if (*map (fromMap, i + diffs[k][0], j + diffs[k][1], xs + 2) == C_HEAD)
						nbHeads++;
				if (nbHeads == 1 || nbHeads == 2)
					*out = C_HEAD;
				else
					*out = C_WIRE;
			} else if (state == C_HEAD) {
				*out = C_TAIL;
			} else { // C_TAIL
				*out = C_WIRE;
			}
		}

	*dir = 1 - *dir;
}
