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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
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

static int add_region(struct _dim *d, int x, int z, const char *ext)
{
	char *fn;
	region_t r;

	if ( asprintf(&fn, "%s/r.%d.%d.%s", d->path, x, z, ext) < 0 )
		goto err;

	r = region_open(fn);
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
	region_put(r);
err_free:
	free(fn);
err:
	return 0;
}

dim_t dim_open(const char *path)
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
	if ( NULL == dir ) {
		if ( errno == ENOENT ) {
			/* treat as empty region */
			goto ok;
		}else{
			goto out_free_path;
		}
	}

	while( (de = readdir(dir)) ) {
		const char *ext;
		int x, z;
		if ( sscanf(de->d_name, "r.%d.%d.mcr", &x, &z) == 2 )
			ext = "mcr";
		else if ( sscanf(de->d_name, "r.%d.%d.mca", &x, &z) == 2 ) {
			ext = "mca";
		}else
			continue;
		if ( !add_region(d, x, z, ext) )
			goto out_free_path;
	}

	closedir(dir);

ok:
	return d;

out_free_path:
	free(d->path);
out_free:
	free(d);
	d = NULL;
out:
	return d;
}

static int have_dir(const char *dir)
{
	if ( !mkdir(dir, 0700) || errno == EEXIST )
		return 1;
	return 0;
}

dim_t dim_create(const char *path)
{
	struct _dim *d;

	d = calloc(1, sizeof(*d));
	if ( NULL == d )
		goto out;

	d->path = strdup(path);
	if ( NULL == d->path )
		goto out_free;

	if ( !have_dir(path) )
		goto out_free;

	goto out; /* success */
out_free:
	free(d->path);
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
			return region_get(d->reg[i].reg);
	}
	return NULL;
}

region_t dim_new_region(dim_t d, int x, int z)
{
	struct dim_reg *dr = NULL;
	unsigned int i;
	char *fn;
	region_t r;

	for(i = 0; i < d->num_reg; i++) {
		if ( d->reg[i].x == x && d->reg[i].z == z ) {
			dr = &d->reg[i];
			break;
		}
	}

	if ( asprintf(&fn, "%s/r.%d.%d.%s", d->path, x, z, "mcr") < 0 )
		return NULL;

	r = region_new(fn);
	free(fn);
	if ( NULL == r )
		return NULL;

	region_set_pos(r, x, z);

	if ( NULL == dr ) {
		if ( !reg_assure(d) ) {
			region_put(r);
			return NULL;
		}
		d->reg[d->num_reg].x = x;
		d->reg[d->num_reg].z = z;
		dr = &d->reg[d->num_reg];
		d->num_reg++;
	}else{
		region_put(dr->reg);
	}

	dr->reg = r;
	return r;
}

void dim_close(dim_t d)
{
	if ( d ) {
		unsigned int i;
		for(i = 0; i < d->num_reg; i++)
			region_put(d->reg[i].reg);
		free(d->reg);
		free(d->path);
		free(d);
	}
}
