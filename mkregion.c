/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
*/
#include <unistd.h>
#include <time.h>

#include <libmc/minecraft.h>
#include <libmc/schematic.h>
#include <libmc/chunk.h>
#include <libmc/region.h>

int main(int argc, char **argv)
{
	unsigned int i, j;
	region_t dst;
	chunk_t c;
	time_t ts = time(NULL);
	int x, z;

	if ( argc < 3 || sscanf(argv[2], "%d,%d", &x, &z) != 2 ) {
		fprintf(stderr, "Usage:\n\t%s <dst> <x,z>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	printf("mkregion called == %s %d,%d\n", argv[1], x, z);

	dst = region_new(argv[1]);
	if ( NULL == dst )
		return EXIT_FAILURE;

	region_set_pos(dst, x, z);

	printf("%s opened for writing\n", argv[1]);

	c = chunk_new();
	if ( NULL == c )
		return EXIT_FAILURE;

	if ( !chunk_set_terrain_populated(c, 1) )
		return EXIT_FAILURE;

	chunk_floor(c, 0, 7);
	chunk_floor(c, 1, 1);
	chunk_floor(c, 2, 1);
	chunk_floor(c, 3, 1);

	printf("Created world floor\n");

	for(i = 0; i < REGION_X; i++) {
		for(j = 0; j < REGION_Z; j++) {
			if ( !region_set_chunk(dst, i, j, c) )
				return EXIT_FAILURE;

			region_set_timestamp(dst, i, j, ts);
			//printf("chunk set to %u,%u in test.mcr\n", i, j);
		}
	}

	chunk_put(c);

	if ( !region_save(dst) )
		return EXIT_FAILURE;

	printf("%s saved\n", argv[1]);

	region_put(dst);
	printf("Done.\n");
	return EXIT_SUCCESS;
}
