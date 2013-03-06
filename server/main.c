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
		int res = serverAccept (serverSock);
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
	uint32_t dummy;
	char * firstMap;

	if (connectionWaitForInit (sock, &xsize, &ysize, &dummy, &firstMap) == 0) {
		// Init double buffers with insulator borders
		char * maps[2];
		int j;
		for (j = 0; j < 2; ++j) {
			maps[j] = malloc ((xsize + 2) * (ysize + 2) * sizeof (char));
			assert (maps[j] != NULL); 

			uint32_t i;
			// horiz lines
			for (i = 0; i < xsize + 2; ++i) {
				*map (maps[j], i, 0, xsize + 2) = C_INSULATOR;
				*map (maps[j], i, ysize + 1, xsize + 2) = C_INSULATOR;
			}
			// vert lines
			for (i = 0; i < ysize + 2; ++i) {
				*map (maps[j], 0, i, xsize + 2) = C_INSULATOR;
				*map (maps[j], xsize + 1, i, xsize + 2) = C_INSULATOR;
			}
		}

		// Copy first frame into maps[0]
		uint32_t x, y;
		for (y = 0; y < ysize; ++y)
			for (x = 0; x < xsize; ++x)
				*map (maps[0], x + 1, y + 1, xsize + 2) = *map (firstMap, x, y, xsize);

		free (firstMap);

		int updatedMap = 0;
		while (1) {
			// Wait R_FRAME
			if (connectionWaitFrameRequest (sock) != 0)
				break;

			// Compute new step
			update_map (&updatedMap, maps, xsize, ysize);

			// Send new map
			if (connectionSendFullUpdate (sock,
						maps[updatedMap], xsize + 2, ysize + 2,
						1, 1, xsize + 1, ysize + 1) != 0)
				break;
		}

		free (maps[0]);
		free (maps[1]);
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

