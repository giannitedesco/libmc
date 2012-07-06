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

#if 0
static void schematic_to_chunk(schematic_t s, chunk_t c)
{
	int x, y, z;
	uint8_t *sb, *sd;
	uint8_t *cb, *cd;
	int16_t sx, sy, sz;
	int zofs = 4;

	schematic_get_size(s, &sx, &sy, &sz);
	printf("schematic %d x %d x %d\n", sx, sy, sz);

	sb = schematic_get_blocks(s);
	sd = schematic_get_data(s);


	cb = chunk_get_blocks(c);
	cd = chunk_get_data(c);

	for(x = 0; x < sx; x++) {
		for(y = 0; y < sy; y++) {
			for(z = 0; z < sz; z++) {
				uint8_t in, *out;
				unsigned int didx;
				uint8_t din;

				in = sb[(y * sz * sx) + (z * sx) + x];
				out = &cb[(x * CHUNK_Y * CHUNK_Z) +
						 (z * CHUNK_Y) + y + zofs];

				*out = in;

				didx = (y * sz * sx) + (z * sx) + x;
				din = sd[didx];

				didx = (x * CHUNK_Y * CHUNK_Z) +
					(z * CHUNK_Y) + y + zofs;
				if ( didx % 2 ) {
					cd[didx/2] = (din << 4) |
							(cd[didx/2] & 0x0f);
				}else{
					cd[didx/2] = (cd[didx/2] & 0xf0) |
							din;
				}
			}
		}
	}
}
#endif

static int mkregion(region_t dst)
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

#if 0
	do {
		schematic_t s;
		s = schematic_load("7seg.schematic");
		if ( s )
			schematic_to_chunk(s, c);
	}while(0);
#endif

	printf("Created world floor\n");

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

	printf("Done.\n");
	return 1;
}
int main(int argc, char **argv)
{
	world_t w;
	level_t l;
	dim_t d;
	region_t r;

	if ( argc < 3) {
		fprintf(stderr, "Usage:\n\t%s <dst> <name>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	printf("mkworld called == %s %s\n", argv[1], argv[2]);
	w = world_create(argv[1]);
	if ( NULL == w )
		return EXIT_FAILURE;

	l = world_get_level(w);

	if ( !level_set_name(l, argv[2]) )
		return EXIT_FAILURE;
	if ( !level_set_spawn(l, 256, 7, 256) )
		return EXIT_FAILURE;

	level_put(l);

	d = world_get_overworld(w);
	r = dim_new_region(d, 0, 0);
	mkregion(r);
	region_put(r);

	printf("Saving world to %s\n", argv[1]);
	if ( !world_save(w) )
		return EXIT_FAILURE;

	world_close(w);
	printf("Done.\n");
	return EXIT_SUCCESS;
}
