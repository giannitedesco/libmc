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
	struct _schematic *l;
	uint8_t *buf;
	size_t sz;

	l = calloc(1, sizeof(*l));
	if ( NULL == l )
		goto out;

	if ( !gunzip(path, &buf, &sz) )
		goto out_free;

	l->nbt = nbt_decode(buf, sz);
	free(buf);
	if ( NULL == l->nbt )
		goto out_free;

	l->schem = nbt_root_tag(l->nbt);
	if ( NULL == l->schem )
		goto out_free;

	l->ref = 1;
	goto out;

out_free:
	nbt_free(l->nbt);
	free(l);
	l = NULL;
out:
	return l;
}

static void schematic_free(schematic_t l)
{
	nbt_free(l->nbt);
	free(l);
}

schematic_t schematic_get(schematic_t l)
{
	l->ref++;
	return l;
}

void schematic_put(schematic_t l)
{
	if ( 0 == --l->ref )
		schematic_free(l);
}
