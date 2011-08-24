/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load each chunk. Chunks contain the actual level data encoded in NBT format
*/
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <zlib.h>

#include <libmc/chunk.h>
#include <libmc/nbt.h>

#define CHUNK_BLOCKS_SIZE (CHUNK_X * CHUNK_Y * CHUNK_Z)
#define CHUNK_DATA_SIZE (CHUNK_BLOCKS_SIZE / 2)

struct _chunk {
	unsigned int ref;
	nbt_t nbt;
	nbt_tag_t level;
};

int chunk_floor(chunk_t c, uint8_t y, unsigned int blk)
{
	unsigned int x, z;
	uint8_t *buf;
	size_t len;

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "Blocks"),
				&buf, &len) )
		return 0;

	for(x = 0; x < CHUNK_X; x++) {
		for(z = 0; z < CHUNK_Z; z++) {
			buf[(x * CHUNK_Y * CHUNK_Z) + (z * CHUNK_Y) + y] = blk;
		}
	}

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "HeightMap"),
				&buf, &len) )
		return 0;
	memset(buf, 0x1, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "SkyLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "BlockLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0x0, len);

	return 1;
}

int chunk_solid(chunk_t c, unsigned int blk)
{
	uint8_t *buf;
	size_t len;

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "Blocks"),
				&buf, &len) )
		return 0;
	memset(buf, blk, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "Data"),
				&buf, &len) )
		return 0;
	memset(buf, 0, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "HeightMap"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "SkyLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "BlockLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0x0, len);

	return 1;
}

uint8_t *chunk_get_blocks(chunk_t c)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "Blocks"), &buf, &sz) )
		return NULL;

	if ( sz != CHUNK_BLOCKS_SIZE ) {
		fprintf(stderr, "chunk: bad blocks size\n");
		return NULL;
	}

	return buf;
}

uint8_t *chunk_get_data(chunk_t c)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_buffer_get(nbt_compound_get(c->level, "Data"), &buf, &sz) )
		return NULL;

	if ( sz != CHUNK_DATA_SIZE ) {
		fprintf(stderr, "chunk: bad blocks size\n");
		return NULL;
	}

	return buf;
}

static uint8_t *chunk_enc_raw(chunk_t c, size_t *sz)
{
	uint8_t *buf;

	*sz = nbt_size_in_bytes(c->nbt);
	buf = malloc(*sz);
	if ( NULL == buf )
		return NULL;

	if ( !nbt_get_bytes(c->nbt, buf, *sz) ) {
		free(buf);
		return NULL;
	}

	return buf;
}

static uint8_t *chunk_enc_zlib(chunk_t c, size_t *sz)
{
	size_t dlen, clen;
	uint8_t *dbuf, *cbuf = NULL;

	dbuf = chunk_enc_raw(c, &dlen);
	if ( NULL == dbuf )
		goto out;

	clen = dlen + 1024;
	cbuf = malloc(clen);
	if ( NULL == cbuf )
		goto out;
	
	if ( compress(cbuf, &clen, dbuf, dlen) == Z_OK ) {
		*sz = clen;
	}else{
		free(cbuf);
		cbuf = NULL;
	}

out:
	free(dbuf);
	return cbuf;
}

uint8_t *chunk_encode(chunk_t c, int enc, size_t *sz)
{
	switch(enc) {
	case CHUNK_ENC_ZLIB:
		return chunk_enc_zlib(c, sz);
	case CHUNK_ENC_RAW:
		return chunk_enc_raw(c, sz);
	default:
		return NULL;
	}
}

int chunk_strip_entities(chunk_t c)
{
	nbt_tag_t ents;
	int rc = 1;

	ents = nbt_compound_get(c->level, "Entities");
	if ( !nbt_list_nuke(ents) )
		rc = 0;

	return rc;
}

int chunk_set_pos(chunk_t c, int32_t x, int32_t z)
{
	if ( !nbt_int_set(nbt_compound_get(c->level, "xPos"), x) )
		return 0;
	if ( !nbt_int_set(nbt_compound_get(c->level, "zPos"), z) )
		return 0;
	return 1;
}

int chunk_set_terrain_populated(chunk_t c, uint32_t p)
{
	return nbt_byte_set(nbt_compound_get(c->level, "TerrainPopulated"), p);
}

static int create_int_keys(nbt_t nbt, nbt_tag_t level)
{
	static const char * const names[] = {
		"LastUpdate",
		"xPos",
		"zPos",
		"TerrainPopulated",
	};
	static const uint8_t types[] = {
		NBT_TAG_Long,
		NBT_TAG_Int,
		NBT_TAG_Int,
		NBT_TAG_Byte,
	};
	unsigned int i;

	for(i = 0; i < sizeof(names)/sizeof(*names); i++) {
		nbt_tag_t tag;
		tag = nbt_tag_new(nbt, types[i]);
		if ( NULL == tag )
			return 0;
		if ( !nbt_compound_set(level, names[i], tag) )
			return 0;
	}
	return 1;
}

static int create_blob_keys(nbt_t nbt, nbt_tag_t level)
{
	static const char * const names[] = {
		"Data",
		"SkyLight",
		"HeightMap",
		"BlockLight",
		"Blocks",
	};
	static const size_t sizes[] = {
		16384U,
		16384U,
		256U,
		16384U,
		32768U,
	};
	size_t max = 32768U;
	unsigned int i;
	uint8_t *buf;
	int rc = 0;

	buf = calloc(1, max);
	if ( NULL == buf )
		return 0;

	for(i = 0; i < sizeof(names)/sizeof(*names); i++) {
		nbt_tag_t tag;
		tag = nbt_tag_new(nbt, NBT_TAG_Byte_Array);
		if ( NULL == tag )
			goto out;
		if ( !nbt_buffer_set(tag, buf, sizes[i]) )
			goto out;
		if ( !nbt_compound_set(level, names[i], tag) )
			goto out;
	}

	rc = 1;

out:
	free(buf);
	return rc;
}

static int create_list_keys(nbt_t nbt, nbt_tag_t level)
{
	static const char * const names[] = {
		"Entities",
		"TileEntities",
	};
	static const uint8_t types[] = {
		NBT_TAG_Compound,
		NBT_TAG_Compound,
	};
	unsigned int i;

	for(i = 0; i < sizeof(names)/sizeof(*names); i++) {
		nbt_tag_t tag;
		tag = nbt_tag_new_list(nbt, types[i]);
		if ( NULL == tag )
			return 0;
		if ( !nbt_compound_set(level, names[i], tag) )
			return 0;
	}
	return 1;
}

static nbt_tag_t create_chunk_keys(nbt_t nbt)
{
	nbt_tag_t level;
	nbt_tag_t root;

	root = nbt_root_tag(nbt);
	if ( NULL == root )
		return NULL;

	level = nbt_tag_new(nbt, NBT_TAG_Compound);
	if ( NULL == level )
		return NULL;

	if ( !nbt_compound_set(root, "Level", level) )
		return NULL;

	if ( !create_int_keys(nbt, level) )
		return NULL;
	if ( !create_blob_keys(nbt, level) )
		return NULL;
	if ( !create_list_keys(nbt, level) )
		return NULL;

	return level;
}

chunk_t chunk_new(void)
{
	struct _chunk *c;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->nbt = nbt_new();
	if ( NULL == c->nbt )
		goto out_free;

	c->level = create_chunk_keys(c->nbt);
	if ( NULL == c->level )
		goto out_free_nbt;

	c->ref = 1;
	goto out;

out_free_nbt:
	nbt_free(c->nbt);
out_free:
	free(c);
	c = NULL;
out:
	return c;
}

chunk_t chunk_from_bytes(uint8_t *buf, size_t sz)
{
	struct _chunk *c;
	nbt_tag_t root;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->nbt = nbt_decode(buf, sz);
	if ( NULL == c->nbt )
		goto out_free;

	root = nbt_root_tag(c->nbt);
	if ( NULL == root )
		goto out_free_nbt;

	c->level = nbt_compound_get(root, "Level");
	if ( NULL == c->level )
		goto out_free_nbt;

	//nbt_dump(c->nbt);
	//printf("decoded %zu bytes of chunk data\n", sz);

	c->ref = 1;
	goto out;

out_free_nbt:
	nbt_free(c->nbt);
out_free:
	free(c);
	c = NULL;
out:
	return c;
}

static void chunk_free(chunk_t c)
{
	nbt_free(c->nbt);
	free(c);
}

void chunk_put(chunk_t c)
{
	if ( 0 == --c->ref )
		chunk_free(c);
}

chunk_t chunk_get(chunk_t c)
{
	c->ref++;
	return c;
}
