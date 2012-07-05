/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
*/
#include <stdint.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <libmc/chunk.h>
#include <libmc/region.h>
#include <libmc/dim.h>
#include <libmc/level.h>
#include <libmc/world.h>

static const char *cmd = "mcdump";

int main(int argc, char **argv)
{
	world_t w;
	dim_t d;
	region_t r;
	uint8_t x, z;

	if ( argc )
		cmd = argv[0];

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <save-game>\n", cmd);
		return EXIT_FAILURE;
	}

	w = world_open(argv[1]);
	if ( w == NULL ) {
		fprintf(stderr, "%s: world_open: failed\n", cmd);
		return EXIT_FAILURE;
	}

	d = world_get_overworld(w);
	if ( NULL == d ) {
		fprintf(stderr, "%s: world_get_overworld: failed\n", cmd);
		return EXIT_FAILURE;
	}

	printf("Opened dimension: %s\n", argv[1]);

	r = dim_get_region(d, 0, 0);
	if ( NULL == r )
		return EXIT_FAILURE;

	printf("Got region 0,0\n");
	for(x = 0; x < REGION_X; x++) {
		for(z = 0; z < REGION_Z; z++) {
			chunk_t c;

			c = region_get_chunk(r, x, z);
			if ( NULL == c )
				continue;

			printf("Got chunk x=%u, z=%u\n", x, z);
			chunk_put(c);
		}
	}

	region_put(r);
	world_close(w);

	return EXIT_SUCCESS;
}
