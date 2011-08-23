/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load entire save-game including level.dat, players and each dimesion (eath +
 * nether).
*/
#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <zlib.h>

#include <libmc/chunk.h>
#include <libmc/region.h>
#include <libmc/dim.h>
#include <libmc/world.h>
#include <libmc/level.h>
#include <libmc/nbt.h>

struct _world {
	dim_t earth;
	dim_t nether;
	level_t level_dat;
};

world_t world_open(const char *dir)
{
	struct _world *w;
	char *path;

	w = calloc(1, sizeof(*w));
	if ( NULL == w )
		goto out;

	if ( asprintf(&path, "%s/level.dat", dir) < 0 )
		goto out_free;
	w->level_dat = level_load(path);
	free(path);
	if ( NULL == w->level_dat )
		goto out_free;

	/* Open regular world */
	if ( asprintf(&path, "%s/region", dir) < 0 )
		goto out_free;
	w->earth = dim_open(path);
	free(path);
	if ( NULL == w->earth )
		goto out_free;

	/* Open nether */
	if ( asprintf(&path, "%s/DIM-1/region", dir) < 0 )
		goto out_free;
	w->nether = dim_open(path);
	free(path);
	if ( NULL == w->nether )
		goto out_free;

	goto out;

out_free:
	level_put(w->level_dat);
	dim_close(w->earth);
	dim_close(w->nether);
	free(w);
	w = NULL;
out:
	return w;
}

world_t world_create(void)
{
	struct _world *w;

	w = calloc(1, sizeof(*w));
	if ( NULL == w )
		goto out;

	w->level_dat = level_new();

	goto out;

out_free:
	level_put(w->level_dat);
	dim_close(w->earth);
	dim_close(w->nether);
	free(w);
	w = NULL;
out:
	return w;
}

dim_t world_get_earth(world_t w)
{
	return w->earth;
}

dim_t world_get_nether(world_t w)
{
	return w->nether;
}

void world_close(world_t w)
{
	if ( w ) {
		level_put(w->level_dat);
		dim_close(w->earth);
		dim_close(w->nether);
		free(w);
	}
}
