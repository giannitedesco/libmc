/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Handle schematic.dat files
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

#include <libmc/nbt.h>
#include <libmc/schematic.h>

struct _schematic {
	nbt_t nbt;
	nbt_tag_t schem;
	int16_t x, y, z;
	unsigned ref;
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

	/* grab decompressed len from gzip trailer, eugh */
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
		goto out;
	}

	*begin = buf;
	*osz = dlen;
	rc = 1;
	goto out;

out_close:
	close(fd);
out:
	return rc;
}

schematic_t schematic_load(const char *path)
{
	struct _schematic *s;
	uint8_t *buf;
	size_t sz;

	s = calloc(1, sizeof(*s));
	if ( NULL == s )
		goto out;

	if ( !gunzip(path, &buf, &sz) )
		goto out_free;

	s->nbt = nbt_decode(buf, sz);
	free(buf);
	if ( NULL == s->nbt )
		goto out_free;

	s->schem = nbt_root_tag(s->nbt);
	if ( NULL == s->schem )
		goto out_free;

	if ( !nbt_short_get(nbt_compound_get(s->schem, "Width"), &s->x) )
		goto out_free;
	if ( !nbt_short_get(nbt_compound_get(s->schem, "Height"), &s->y) )
		goto out_free;
	if ( !nbt_short_get(nbt_compound_get(s->schem, "Length"), &s->z) )
		goto out_free;

	if ( s->x < 0 || s->y < 0 || s->z < 0 )
		goto out_free;

	s->ref = 1;
	goto out;

out_free:
	nbt_free(s->nbt);
	free(s);
	s = NULL;
out:
	return s;
}

static void schematic_free(schematic_t s)
{
	nbt_free(s->nbt);
	free(s);
}

schematic_t schematic_get(schematic_t s)
{
	s->ref++;
	return s;
}

void schematic_put(schematic_t s)
{
	if ( 0 == --s->ref )
		schematic_free(s);
}

void schematic_get_size(schematic_t s, int16_t *x, int16_t *y, int16_t *z)
{
	if ( x )
		*x = s->x;
	if ( y )
		*y = s->y;
	if ( z )
		*z = s->z;
}

uint8_t *schematic_get_blocks(schematic_t s)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_bytearray_get(nbt_compound_get(s->schem, "Blocks"), &buf, &sz) )
		return NULL;

	if ( sz != (size_t)((s->x * s->y * s->z)) ) {
		fprintf(stderr, "schematic: bad blocks size\n");
		return NULL;
	}

	return buf;
}

uint8_t *schematic_get_data(schematic_t s)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_bytearray_get(nbt_compound_get(s->schem, "Data"), &buf, &sz) )
		return NULL;

	if ( sz != (size_t)(s->x * s->y * s->z) ) {
		fprintf(stderr, "schematic: bad blocks size\n");
		return NULL;
	}

	return buf;
}
