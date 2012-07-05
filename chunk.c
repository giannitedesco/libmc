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

#define CHUNK_NUM_SECTIONS	16
#define CHUNK_SECTION_Y		(CHUNK_Y / CHUNK_NUM_SECTIONS)

struct _chunk {
	unsigned int ref;
	nbt_t nbt;
	nbt_tag_t level;
	nbt_tag_t seclist;
	nbt_tag_t section[CHUNK_NUM_SECTIONS];
};

static int create_section_blobs(nbt_t nbt, nbt_tag_t sec)
{
	static const char * const names[] = {
		"Data",
		"SkyLight",
		"BlockLight",
		"Blocks",
	};
	static const size_t sizes[] = {
		2048U,
		2048U,
		2048U,
		4096U,
	};
	size_t max = 4096U;
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
		if ( !nbt_bytearray_set(tag, buf, sizes[i]) )
			goto out;
		if ( !nbt_compound_set(sec, names[i], tag) )
			goto out;
	}

	rc = 1;

out:
	free(buf);
	return rc;
}

static nbt_tag_t get_add_section(chunk_t c, uint8_t secno)
{
	if ( NULL == c->seclist ) {
		nbt_tag_t list;
		list = nbt_tag_new_list(c->nbt, NBT_TAG_Compound);
		if ( NULL == list )
			return NULL;

		printf("HAI\n");
		if ( !nbt_compound_set(c->level, "Sections", list) )
			return NULL;

		c->seclist = list;
	}

	if ( NULL == c->section[secno] ) {
		nbt_tag_t sec, ytag;

		sec = nbt_tag_new(c->nbt, NBT_TAG_Compound);
		if ( NULL == sec )
			return NULL;

		if ( !nbt_list_append(c->seclist, sec) )
			return NULL;

		if ( !create_section_blobs(c->nbt, sec) )
			return NULL;

		ytag = nbt_tag_new(c->nbt, NBT_TAG_Byte);
		if ( NULL == ytag )
			return NULL;
		if ( !nbt_byte_set(ytag, secno) )
			return NULL;
		if ( !nbt_compound_set(sec, "Y", ytag) )
			return NULL;

		c->section[secno] = sec;
	}

	return c->section[secno];
}

int chunk_floor(chunk_t c, uint8_t y, unsigned int blk)
{
	unsigned int x, z, i, num;
	uint8_t *buf, secno, ty;
	int32_t *hm;
	nbt_tag_t s;
	size_t len;

	secno = y / CHUNK_NUM_SECTIONS;

	s = get_add_section(c, secno);
	if ( NULL == s )
		return 0;

	ty = y - secno * CHUNK_SECTION_Y;

	if ( !nbt_bytearray_get(nbt_compound_get(s, "Blocks"),
				&buf, &len) )
		return 0;

	for(x = 0; x < CHUNK_X; x++) {
		for(z = 0; z < CHUNK_Z; z++) {
			buf[(ty * CHUNK_Z * CHUNK_X) + (z * CHUNK_X) + x] = blk;
		}
	}

	if ( !nbt_intarray_get(nbt_compound_get(c->level, "HeightMap"),
				&hm, &num) )
		return 0;
	for(i = 0; i < num; i++) {
		hm[i] = htobe32(y);
	}

	if ( !nbt_bytearray_get(nbt_compound_get(s, "SkyLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	return 1;
}

int chunk_solid(chunk_t c, unsigned int blk)
{
	uint8_t *buf;
	size_t len;

	if ( !nbt_bytearray_get(nbt_compound_get(c->level, "Blocks"),
				&buf, &len) )
		return 0;
	memset(buf, blk, len);

	if ( !nbt_bytearray_get(nbt_compound_get(c->level, "HeightMap"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	return 1;
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

int chunk_set_terrain_populated(chunk_t c, uint8_t p)
{
	nbt_tag_t tag;
	tag = nbt_compound_get(c->level, "TerrainPopulated");
	if ( NULL == tag ) {
		tag = nbt_tag_new(c->nbt, NBT_TAG_Byte);
		if ( NULL == tag )
			return 0;
		if ( !nbt_compound_set(c->level, "TerrainPopulated", tag) )
			return 0;
	}
	return nbt_byte_set(tag, p);
}

static int create_int_keys(nbt_t nbt, nbt_tag_t level)
{
	static const char * const names[] = {
		"LastUpdate",
		"xPos",
		"zPos",
	};
	static const uint8_t types[] = {
		NBT_TAG_Long,
		NBT_TAG_Int,
		NBT_TAG_Int,
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
		"Biomes",
	};
	static const size_t sizes[] = {
		256U,
	};
	size_t max = 256U;
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
		if ( !nbt_bytearray_set(tag, buf, sizes[i]) )
			goto out;
		if ( !nbt_compound_set(level, names[i], tag) )
			goto out;
	}

	rc = 1;

out:
	free(buf);
	return rc;
}

static int create_int_array_keys(nbt_t nbt, nbt_tag_t level)
{
	static const char * const names[] = {
		"HeightMap",
	};
	static const size_t sizes[] = {
		256U,
	};
	size_t max = 256U;
	unsigned int i;
	int32_t *buf;
	int rc = 0;

	buf = calloc(1, max);
	if ( NULL == buf )
		return 0;

	for(i = 0; i < sizeof(names)/sizeof(*names); i++) {
		nbt_tag_t tag;
		tag = nbt_tag_new(nbt, NBT_TAG_Int_Array);
		if ( NULL == tag )
			goto out;
		if ( !nbt_intarray_set(tag, buf, sizes[i]) )
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
		"TileEntities",
		"Entities",
//		"Sections",
	};
	static const uint8_t types[] = {
		NBT_TAG_Byte,
		NBT_TAG_Byte,
//		NBT_TAG_Compound,
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
	if ( !create_int_array_keys(nbt, level) )
		return NULL;
	if ( !create_list_keys(nbt, level) )
		return NULL;
//	if ( !create_blob_keys(nbt, level) )
//		return NULL;

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

#if 0
	/* TODO: read section list */
	nbt_tag_t s, tag;

	for(i = 0; i < CHUNK_NUM_SECTIONS; i++) {
		uint8_t val;
		s = c->section[i];
		tag = nbt_compound_get(s, "Y");
		if ( !nbt_byte_get(tag, &val) ) {
			abort();
		}

		if ( val == secno )
			return s;
	}
#endif
	nbt_dump(c->nbt);
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
