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
#include <assert.h>

#include <zlib.h>

#include <libmc/schematic.h>
#include <libmc/chunk.h>
#include <libmc/nbt.h>

#define CHUNK_BLOCKS_SIZE (CHUNK_X * CHUNK_Y * CHUNK_Z)
#define CHUNK_DATA_SIZE (CHUNK_BLOCKS_SIZE / 2)

#define CHUNK_NUM_SECTIONS	16
#define CHUNK_SECTION_Y		(CHUNK_Y / CHUNK_NUM_SECTIONS)

#define SEC_FLOOR(y) (y / CHUNK_SECTION_Y)
#define SEC_CEIL(y) ((y + (CHUNK_SECTION_Y - 1)) / CHUNK_SECTION_Y)

struct _chunk {
	unsigned int ref;
	nbt_t nbt;
	nbt_tag_t level;
	nbt_tag_t seclist;
	nbt_tag_t section[CHUNK_NUM_SECTIONS];
	unsigned int dirty_mask;
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

	c->dirty_mask |= (1 << secno);
	return c->section[secno];
}

static void clear_dirty_section(nbt_tag_t s)
{
	uint8_t *buf;
	size_t len;

	if ( nbt_bytearray_get(nbt_compound_get(s, "SkyLight"),
						&buf, &len) )
		memset(buf, 0xff, len);
	if ( nbt_bytearray_get(nbt_compound_get(s, "BlockLight"),
						&buf, &len) )
		memset(buf, 0xff, len);
}


static void clear_dirty(struct _chunk *c)
{
	unsigned int i, num;
	int32_t *hm;

	if ( nbt_intarray_get(nbt_compound_get(c->level, "HeightMap"),
						&hm, &num) ) {
		for(i = 0; i < num; i++) {
			hm[i] = htobe32(0xff);
		}
	}

	for(i = 0; i < CHUNK_NUM_SECTIONS; i++) {
		if ( c->section[i] ) {
			clear_dirty_section(c->section[i]);
		}
	}

	c->dirty_mask = 0;
}

int chunk_floor(chunk_t c, uint8_t y, unsigned int blk)
{
	unsigned int x, z;
	uint8_t *buf, secno, ty;
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
	clear_dirty(c);

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
	};
	static const uint8_t types[] = {
		NBT_TAG_Byte,
		NBT_TAG_Byte,
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
	nbt_tag_t s, tag;
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

	c->seclist = nbt_compound_get(c->level, "Sections");
	if ( c->seclist ) {
		int i, num = nbt_list_get_size(c->seclist);
		for(i = 0; i < num; i++) {
			uint8_t val;
			s = nbt_list_get(c->seclist, i);
			if ( NULL == s )
				continue;

			tag = nbt_compound_get(s, "Y");
			if ( !nbt_byte_get(tag, &val) ) {
				abort();
			}

			if ( val < CHUNK_NUM_SECTIONS )
				c->section[val] = s;
		}
	}

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

int chunk_paste_schematic(chunk_t c, schematic_t s, int x, int y, int z)
{
	int16_t sx, sz, sy;
	int xmin, ymin, zmin;
	int xmax, ymax, zmax;
	int tx, ty, tz;
	int cx, cy, cz;
	int smin, smax, i, ci = 0;
	uint8_t *sb, *sd;

	schematic_get_size(s, &sx, &sy, &sz);
	tx = x + sx;
	ty = y + sy;
	tz = z + sz;

	xmin = d_min(x, tx);
	ymin = d_min(y, ty);
	zmin = d_min(z, tz);
	xmax = d_max(x, tx);
	ymax = d_max(y, ty);
	zmax = d_max(z, tz);

	printf("chunk: schematic dimesions %d,%d,%d -> %d,%d,%d\n",
		xmin, ymin, zmin, xmax, ymax, zmax);

	assert(xmin >= 0 && ymin >= 0 && zmin >= 0);
	assert(xmax < CHUNK_X && ymax < CHUNK_Y && zmax < CHUNK_Z);

	smin = SEC_FLOOR(ymin);
	smax = SEC_CEIL(ymax);

	sb = schematic_get_blocks(s);
	sd = schematic_get_data(s);

	for(i = smin; i < smax; i++, y -= CHUNK_SECTION_Y) {
		int tmin, tmax, co;
		nbt_tag_t sec;
		uint8_t *cb, *cd;
		size_t len;

		tmin = i * CHUNK_SECTION_Y;
		tmax = (i + 1) * CHUNK_SECTION_Y;
		tmin = d_max(tmin, ymin);
		tmax = d_min(tmax, ymax);
		co = tmin % CHUNK_SECTION_Y;

		sec = get_add_section(c, i);
		if ( NULL == sec )
			return 0;

		if ( !nbt_bytearray_get(nbt_compound_get(sec, "Blocks"),
					&cb, &len) )
			return 0;
		if ( !nbt_bytearray_get(nbt_compound_get(sec, "Data"),
					&cd, &len) )
			return 0;

		for(cy = 0; cy < (tmax - tmin); cy++, ci++, co++) {
			for(cx = 0; cx < sx; cx++) {
				for(cz = 0; cz < sz; cz++) {
					uint8_t in, *out;
					unsigned int didx;
					uint8_t din;

					in = sb[(ci * sz * sx) +
						(cz * sx) + cx];
					out = &cb[(co * CHUNK_Z * CHUNK_X) +
						(cz * CHUNK_X) + cx];
					*out = in;

					didx = (ci * sz * sx) +
						(cz * sx) + cx;
					din = sd[didx];
					didx = (co * CHUNK_Z * CHUNK_X) +
						(cz * CHUNK_X) + cx;
					if ( didx % 2 ) {
						cd[didx / 2] = (din << 4) |
							(cd[didx/2] & 0x0f);
					}else{
						cd[didx / 2] = (cd[didx/2] &
								0xf0) | din;
					}
				}
			}
		}
	}

	return 1;
}
