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
	chunk_t c;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <region>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	src = region_open(argv[1], 1);
	if ( NULL == src )
		return EXIT_FAILURE;
	
	printf("%s opened for reading\n", argv[1]);

	c = region_get_chunk(src, 0, 0);
	if ( NULL == c )
		return EXIT_FAILURE;

	printf("fetched out chunk at 0,0\n");

	dst = region_new("test.mcr");
	if ( NULL == dst )
		return EXIT_FAILURE;

	printf("test.mcr opened for writing\n");

	if ( !region_set_chunk(dst, 0, 0, c) )
		return EXIT_FAILURE;
	
	printf("chunk set to 0,0 in test.mcr\n");

	if ( !region_save(dst) )
		return EXIT_FAILURE;

	printf("test.mcr saved\n");

	region_close(src);
	region_close(dst);
	printf("Done.\n");
	return EXIT_SUCCESS;
}
