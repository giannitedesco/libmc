/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Handle level.dat files
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
#include <libmc/level.h>

struct _level {
	nbt_t nbt;
	nbt_tag_t data;
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

level_t level_load(const char *path)
{
	struct _level *l;
	nbt_tag_t root;
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

	root = nbt_root_tag(l->nbt);
	if ( NULL == root )
		goto out_free;

	l->data = nbt_compound_get(root, "Data");
	if ( NULL == l->data )
		goto out_free;

	nbt_dump(l->nbt);
	l->ref = 1;
	goto out;

out_free:
	nbt_free(l->nbt);
	free(l);
	l = NULL;
out:
	return l;
}

static int create_int_keys(nbt_t nbt, nbt_tag_t data)
{
	static const char * const names[] = {
		"thundering",
		"LastPlayed",
		"RandomSeed",
		"version",
		"Time",
		"raining",
		"SpawnX",
		"thunderTime",
		"SpawnY",
		"SpawnZ",
		"SizeOnDisk",
		"rainTime",
	};
	static const uint8_t types[] = {
		NBT_TAG_Byte,
		NBT_TAG_Long,
		NBT_TAG_Long,
		NBT_TAG_Int,
		NBT_TAG_Long,
		NBT_TAG_Byte,
		NBT_TAG_Int,
		NBT_TAG_Int,
		NBT_TAG_Int,
		NBT_TAG_Int,
		NBT_TAG_Long,
		NBT_TAG_Int,
	};
	unsigned int i;

	for(i = 0; i < sizeof(names)/sizeof(*names); i++) {
		nbt_tag_t tag;
		tag = nbt_tag_new(nbt, types[i]);
		if ( NULL == tag )
			return 0;
		if ( !nbt_compound_set(data, names[i], tag) )
			return 0;
	}
	return 1;
}

static nbt_tag_t create_data_keys(nbt_t nbt)
{
	nbt_tag_t data;
	nbt_tag_t root;
	nbt_tag_t name;

	root = nbt_root_tag(nbt);
	if ( NULL == root )
		return NULL;
	
	data = nbt_tag_new(nbt, NBT_TAG_Compound);
	if ( NULL == data )
		return NULL;
	
	if ( !nbt_compound_set(root, "Data", data) )
		return NULL;
	
	if ( !create_int_keys(nbt, data) )
		return NULL;

	name = nbt_tag_new(nbt, NBT_TAG_String);
	if ( NULL == name )
		return NULL;
	
	if ( !nbt_compound_set(data, "LevelName", name) )
		return NULL;

	return data;
}

level_t level_new(void)
{
	struct _level *l;

	l = calloc(1, sizeof(*l));
	if ( NULL == l )
		goto out;

	l->nbt = nbt_new();
	if ( NULL == l->nbt )
		goto out_free;

	l->data = create_data_keys(l->nbt);
	if ( NULL == l->data )
		goto out_free;
	
	nbt_dump(l->nbt);
	l->ref = 1;
	goto out;

out_free:
	nbt_free(l->nbt);
	free(l);
	l = NULL;
out:
	return l;
}

static void level_free(level_t l)
{
	nbt_free(l->nbt);
	free(l);
}

level_t level_get(level_t l)
{
	l->ref++;
	return l;
}

void level_put(level_t l)
{
	printf("ref = %u\n", l->ref);
	if ( 0 == --l->ref )
		level_free(l);
}
