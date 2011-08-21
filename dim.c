/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load each dimesion. Dimesions contain multiple regions.
*/
#define _GNU_SOURCE
#include <stdint.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>

#include <endian.h>

#include <zlib.h>

#include <libmc/chunk.h>
#include <libmc/region.h>
#include <libmc/dim.h>

#define DIM_ALLOC_CHUNK (1<<4)
#define DIM_ALLOC_MASK (DIM_ALLOC_CHUNK - 1)

struct dim_reg {
	region_t reg;
	int x;
	int z;
};

struct _dim {
	char *path;
	unsigned int num_reg;
	struct dim_reg *reg;
};

static int reg_assure(struct _dim *d)
{
	void *new;

	if ( d->num_reg & DIM_ALLOC_MASK )
		return 1;

	new = realloc(d->reg,
			sizeof(*d->reg) *
			(d->num_reg + DIM_ALLOC_CHUNK));
	if ( new == NULL )
		return 0;

	d->reg = new;
	return 1;
}

static int add_region(struct _dim *d, int x, int z, int rdonly)
{
	char *fn;
	region_t r;

	if ( asprintf(&fn, "%s/r.%d.%d.mcr", d->path, x, z) < 0 )
		goto err;

	r = region_open(fn, rdonly);
	if ( NULL == r )
		goto err_free;

	if ( !reg_assure(d) )
		goto err_close;

	d->reg[d->num_reg].x = x;
	d->reg[d->num_reg].z = z;
	d->reg[d->num_reg].reg = r;
	d->num_reg++;

	free(fn);
	return 1;

err_close:
	region_close(r);
err_free:
	free(fn);
err:
	return 0;
}
				
dim_t dim_open(const char *path, int rdonly)
{
	struct _dim *d;
	struct dirent *de;
	DIR *dir;

	d = calloc(1, sizeof(*d));
	if ( NULL == d )
		goto out;
	
	d->path = strdup(path);
	if ( NULL == d->path )
		goto out_free;

	dir = opendir(d->path);
	if ( NULL == dir )
		goto out_free_path;

	while( (de = readdir(dir)) ) {
		int x, z;
		if ( sscanf(de->d_name, "r.%d.%d.mcr", &x, &z) != 2 )
			continue;
		if ( !add_region(d, x, z, rdonly) )
			goto out_free_path;
	}

	closedir(dir);

	return d;

out_free_path:
	free(d->path);
out_free:
	free(d);
	d = NULL;
out:
	return d;
}

region_t dim_get_region(dim_t d, int x, int z)
{
	unsigned int i;

	for(i = 0; i < d->num_reg; i++) {
		if ( d->reg[i].x == x && d->reg[i].z == z )
			return d->reg[i].reg;
	}
	return NULL;
}

void dim_close(dim_t d)
{
	if ( d ) {
		unsigned int i;
		for(i = 0; i < d->num_reg; i++)
			region_close(d->reg[i].reg);
		free(d->reg);
		free(d->path);
		free(d);
	}
}
