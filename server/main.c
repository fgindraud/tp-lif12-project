#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "../protocol.h"

/* Config */
#define PORT 8000

/* Proto/macro */
#define sys_assert(cond, text) if (!(cond)) { perror (text); exit (EXIT_FAILURE); }
void perform_simulation (int sock);
int read_words (int sock, wireworld_message_t * buffer, uint32_t count);
int send_words (int sock, wireworld_message_t * buffer, uint32_t count);
void network_to_map (wireworld_message_t * n_map, char * l_map, uint32_t xsize, uint32_t ysize);
void map_to_network (wireworld_message_t * n_map, char * l_map, uint32_t xsize, uint32_t ysize);
int wait_for_init (int sock, uint32_t * xs, uint32_t * ys, char ** maps);
void update_map (int * dir, char ** maps, uint32_t xs, uint32_t ys);

/* Small utils */
char * map (char * tab, int x, int y, int xsize) { return &tab[x + y * xsize]; }

/* main */
int main (void) {
	// Avoid program termination on failed write
	sys_assert (signal (SIGPIPE, SIG_IGN) != SIG_ERR, "signal"); 

	int sock = socket (AF_INET6, SOCK_STREAM, 0);
	sys_assert (sock != -1, "socket");

	struct sockaddr_in6 saddr;

	memset (&saddr, 0, sizeof (saddr));
	saddr.sin6_family = AF_INET6;
	saddr.sin6_port = htons (PORT);
	saddr.sin6_addr = in6addr_any;

	int res = bind (sock, (const struct sockaddr*) &saddr, sizeof (saddr));
	sys_assert (res != -1, "bind");
		
	sys_assert (listen (sock, 5) != -1, "listen");

	while (1) {
		res = accept (sock, NULL, NULL);
		sys_assert (res != -1, "accept");
		
		perform_simulation (res);
		close (res);
	}
	
	return EXIT_SUCCESS;
}

/* Read write utils, with endiannes conversion */
int read_words (int sock, wireworld_message_t * buffer, uint32_t count) {
	uint32_t bytes_to_read = count * sizeof (wireworld_message_t);
	wireworld_message_t * tmp_buf = malloc (bytes_to_read);
	assert (tmp_buf != NULL);
	
	// Read raw data
	char * it = (char *) tmp_buf;
	while (bytes_to_read > 0) {
		int res = read (sock, it, bytes_to_read);
		if (res <= 0) {
			if (res == -1)
				perror ("read");
			else
				fprintf (stderr, "Connection closed\n");
			free (tmp_buf);
			return -1;
		}	
		it += res;
		bytes_to_read -= res;
	}

	// Convert endianness
	uint32_t i;
	for (i = 0; i < count; ++i)
		buffer[i] = ntohl (tmp_buf[i]);

	free (tmp_buf);

	return 0;
}

int send_words (int sock, wireworld_message_t * buffer, uint32_t count) {
	uint32_t bytes_to_send = count * sizeof (wireworld_message_t);
	wireworld_message_t * tmp_buf = malloc (bytes_to_send);
	assert (tmp_buf != NULL);
	
	// Convert endianness
	uint32_t i;
	for (i = 0; i < count; ++i)
		tmp_buf[i] = htonl (buffer[i]);
	
	// Read raw data
	char * it = (char *) tmp_buf;
	while (bytes_to_send > 0) {
		int res = write (sock, it, bytes_to_send);
		if (res == -1) {
				perror ("write");
			free (tmp_buf);
			return -1;
		}	
		it += res;
		bytes_to_send -= res;
	}
	free (tmp_buf);
	return 0;
}

/* network to char map */
void network_to_map (wireworld_message_t * n_map, char * l_map, uint32_t xsize, uint32_t ysize) {
	uint32_t messageIndex = 0;
	int bitIndex = 0;
	uint32_t i, j;
	for (i = 1; i < ysize + 1 ; ++i) {
		for (j = 1; j < xsize + 1; ++j) {
			// Set value
			*map (l_map, j, i, xsize + 2) = C_BIT_MASK & 
				(n_map[messageIndex] >> (C_BIT_SIZE * bitIndex));
			// Update counters
			bitIndex++;
			if (bitIndex > 15) {
				messageIndex++;
				bitIndex = 0;
			}
		}
	}
}

void map_to_network (wireworld_message_t * n_map, char * l_map, uint32_t xsize, uint32_t ysize) {
	uint32_t messageIndex = 0;
	int bitIndex = 0;
	uint32_t i, j;
	
	n_map[0] = 0;
	for (i = 1; i < ysize + 1; ++i) {
		for (j = 1; j < xsize + 1; ++j) {
			// Set value
			n_map[messageIndex] |=
				*map (l_map, j, i, xsize + 2) <<
				(C_BIT_SIZE * bitIndex);
			// Update counters, and init next word
			bitIndex++;
			if (bitIndex > 15) {
				messageIndex++;
				n_map[messageIndex] = 0;
				bitIndex = 0;
			}
		}
	}
}

/* simulation */
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
