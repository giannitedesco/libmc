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

#include <libmc/schematic.h>
#include <libmc/chunk.h>
#include <libmc/region.h>
#include <libmc/dim.h>
#include <libmc/level.h>
#include <libmc/world.h>

static int region_init(region_t dst)
{
	unsigned int i, j;
	chunk_t c;
	time_t ts = time(NULL);

	c = chunk_new();
	if ( NULL == c )
		return 0;

	if ( !chunk_set_terrain_populated(c, 1) )
		return 0;

	for(i = 0; i < 12; i++) {
		chunk_floor(c, i, (i == 0) ? 7 : 24);
	}

	for(i = 0; i < REGION_X; i++) {
		for(j = 0; j < REGION_Z; j++) {
			if ( !region_set_chunk(dst, i, j, c) )
				return 0;

			region_set_timestamp(dst, i, j, ts);
			//printf("chunk set to %u,%u in test.mcr\n", i, j);
		}
	}

	chunk_put(c);

	if ( !region_save(dst) )
		return 0;

	return 1;
}

static int set_level_data(world_t w, const char *name)
{
	level_t l;
	l = world_get_level(w);

	if ( !level_set_name(l, name) )
		return 0;
//	if ( !level_set_spawn(l, 16, 7, 16) )
	if ( !level_set_spawn(l, 250, 12, 250) )
		return 0;

	level_put(l);
	return 1;
}

static int mkregion(dim_t d, int x, int z)
{
	region_t r;

	printf("Creating region %d, %d\n", x, z);
	r = dim_new_region(d, x, z);
	if ( NULL == r)
		return 0;

	if ( !region_init(r) )
		return 0;

	region_put(r);
	return 1;
}

static int do_mkworld(const char *path, const char *name)
{
	schematic_t s;
	world_t w;
	dim_t d;

	w = world_create(path);
	if ( NULL == w )
		return 0;

	if ( !set_level_data(w, name) )
		return 0;

	d = world_get_overworld(w);

	mkregion(d, 0, 0);
	mkregion(d, 1, 1);
	mkregion(d, 0, -1);
	mkregion(d, -1, 0);
	mkregion(d, -1, -1);

	s = schematic_load("7seg-lamps.schematic");
	if ( s ) {
		printf("loaded schematic\n");
		dim_paste_schematic(d, s, 256, 12, 256);
		schematic_put(s);
	}

	printf("Saving world to %s\n", path);
	if ( !world_save(w) )
		return 0;

	world_close(w);
	return 1;
}

int main(int argc, char **argv)
{
	if ( argc < 3) {
		fprintf(stderr, "Usage:\n\t%s <dst> <name>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	printf("mkworld called == %s %s\n", argv[1], argv[2]);
	if ( !do_mkworld(argv[1], argv[2]) )
		return EXIT_FAILURE;

	printf("Done.\n");
	return EXIT_SUCCESS;
}
