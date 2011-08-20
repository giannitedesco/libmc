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

int main(int argc, char **argv)
{
	dim_t d;
	region_t r;
	uint8_t x, z;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <region>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	d = dim_open(argv[1]);
	if ( NULL == d )
		return EXIT_FAILURE;

	printf("Opened dimension: %s\n", argv[1]);

	//r = region_open(argv[1]);
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

			//printf("Got chunk x=%u, z=%u\n", x, z);
			chunk_free(c);
		}
	}

	dim_close(d);

	return EXIT_SUCCESS;
}
