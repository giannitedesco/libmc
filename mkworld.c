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
#include <time.h>

#include <libmc/chunk.h>
#include <libmc/region.h>
#include <libmc/dim.h>
#include <libmc/level.h>
#include <libmc/world.h>

int main(int argc, char **argv)
{
	world_t w;
	level_t l;

	if ( argc < 3) {
		fprintf(stderr, "Usage:\n\t%s <dst> <name>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	printf("mkworld called == %s %s\n", argv[1], argv[2]);
	w = world_create();
	if ( NULL == w )
		return EXIT_FAILURE;

	l = world_get_level(w);

	if ( !level_set_name(l, argv[2]) )
		return EXIT_FAILURE;
	if ( !level_set_spawn(l, 0, 4, 0) )
		return EXIT_FAILURE;

	level_put(l);

	printf("Saving world to %s\n", argv[1]);
	if ( !world_save(w, argv[1]) )
		return EXIT_FAILURE;

	world_close(w);
	printf("Done.\n");
	return EXIT_SUCCESS;
}
