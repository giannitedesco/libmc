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
#include <libmc/world.h>

int main(int argc, char **argv)
{
	world_t w;
	dim_t d;
	region_t r;
	uint8_t x, z;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <save-game>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	w = world_open(argv[1]);
	if ( w == NULL )
		return EXIT_FAILURE;

	d = world_get_earth(w);
	if ( NULL == d )
		return EXIT_FAILURE;

	printf("Opened dimension: %s\n", argv[1]);

	r = dim_get_region(d, 0, -1);
	if ( NULL == r )
		return EXIT_FAILURE;

	printf("Got region 0,0\n");
	for(x = 0; x < REGION_X; x++) {
		for(z = 0; z < REGION_Z; z++) {
			chunk_t c;

			c = region_get_chunk(r, x, z);
			if ( NULL == c )
				continue;

			//printf("Got chunk x=%u, z=%u\n", x, z);
			chunk_put(c);
		}
	}

	region_put(r);
	world_close(w);

	return EXIT_SUCCESS;
}
