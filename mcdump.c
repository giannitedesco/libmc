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
	region_t r;
	uint8_t x, z;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <region>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	r = region_open(argv[1]);
	if ( NULL == r )
		return EXIT_FAILURE;

	printf("Opened region: %s\n", argv[1]);

	for(x = 0; x < REGION_X; x++) {
		for(z = 0; z < REGION_Z; z++) {
			chunk_t c;

			c = region_get_chunk(r, x, z);
			if ( NULL == c )
				continue;

			printf("Got region x=%u, z=%u\n", x, z);
			chunk_free(c);
		}
	}

	region_close(r);

	return EXIT_SUCCESS;
}
