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
#include <libmc/nbt.h>

struct _world {
	dim_t earth;
	dim_t nether;
	nbt_t level_dat;
};

static int gunzip(const char *path, uint8_t **begin, size_t *osz)
{
	int fd, ret, rc = 0;
	uint8_t *buf;
	size_t dlen;
	uint32_t d;
	gzFile gz;

	/* gcc, huh?! */
	*begin = NULL;
	*osz = 0;

	fd = open(path, O_RDONLY);
	if ( fd < 0 )
		goto out;

	/* grap decompressed len from gzip trailer, eugh */
	if ( lseek(fd, -sizeof(d), SEEK_END) < 0 )
		goto out_close;
	if ( read(fd, &d, sizeof(d)) != sizeof(d) )
		goto out_close;
	if ( lseek(fd, 0, SEEK_SET) < 0 )
		goto out_close;

	dlen = d;

	buf = malloc(dlen);
	if ( NULL == buf )
		goto out_close;

	gz = gzdopen(fd, "r");
	ret = gzread(gz, buf, dlen);
	gzclose(gz);
	if ( ret < 0 ) {
		free(buf);
		goto out_close;
	}

	*begin = buf;
	*osz = dlen;
	rc = 1;

out_close:
	close(fd);
out:
	return rc;
}

static nbt_t load_level_dat(const char *path)
{
	uint8_t *buf;
	size_t sz;
	nbt_t nbt;

	if ( !gunzip(path, &buf, &sz) )
		return NULL;

	nbt = nbt_decode(buf, sz);
	free(buf);
	if ( nbt )
		nbt_dump(nbt);
	return nbt;
}

world_t world_open(const char *dir)
{
	struct _world *w;
	char *path;

	w = calloc(1, sizeof(*w));
	if ( NULL == w )
		goto out;

	if ( asprintf(&path, "%s/level.dat", dir) < 0 )
		goto out_free;
	w->level_dat = load_level_dat(path);
	if ( NULL == w->level_dat )
		goto out_free_path;
	free(path);

	/* Open regular world */
	if ( asprintf(&path, "%s/region", dir) < 0 )
		goto out_free;
	w->earth = dim_open(path);
	if ( NULL == w->earth )
		goto out_free_path;
	free(path);

	/* Open nether */
	if ( asprintf(&path, "%s/DIM-1/region", dir) < 0 )
		goto out_free;
	w->nether = dim_open(path);
	free(path);

	goto out;

out_free_path:
	free(path);
out_free:
	nbt_free(w->level_dat);
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
		nbt_free(w->level_dat);
		dim_close(w->earth);
		dim_close(w->nether);
		free(w);
	}
}
