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

int main(int argc, char **argv)
{
	region_t src, dst;
	unsigned int i, j;
	chunk_t c;

	if ( argc < 3 ) {
		fprintf(stderr, "Usage:\n\t%s <src> <dst>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	src = region_open(argv[1]);
	if ( NULL == src )
		return EXIT_FAILURE;
	
	printf("%s opened for reading\n", argv[1]);

	dst = region_new(argv[2]);
	if ( NULL == dst )
		return EXIT_FAILURE;

	printf("%s opened for writing\n", argv[2]);

	for(i = 0; i < REGION_X; i++) {
		for(j = 0; j < REGION_Z; j++) {
			//printf("fetched out chunk at %u,%u\n", i, j);

			c = region_get_chunk(src, i, j);
			if ( NULL == c )
				continue;

#if 0
			if ( i == 0 && j == 0 && !chunk_solid(c, 56) )
				return EXIT_FAILURE;
			printf("Set to solid diamond ore\n");
#endif

			chunk_strip_entities(c);

			if ( !region_set_chunk(dst, i, j, c) )
				return EXIT_FAILURE;

			region_set_timestamp(dst, i, j,
					region_get_timestamp(src, i, j));
			//printf("chunk set to %u,%u in test.mcr\n", i, j);
			chunk_put(c);
		}
	}

	if ( !region_save(dst) )
		return EXIT_FAILURE;

	printf("%s saved\n", argv[2]);

	region_put(src);
	region_put(dst);
	printf("Done.\n");
	return EXIT_SUCCESS;
}
