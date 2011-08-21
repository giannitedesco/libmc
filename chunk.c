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

struct _chunk {
	nbt_t nbt;
	nbt_tag_t level;
};

int chunk_solid(chunk_t c, unsigned int blk)
{
	uint8_t *buf;
	size_t len;

	if ( !nbt_buffer_get(nbt_compound_get_child(c->level, "Blocks"),
				&buf, &len) )
		return 0;
	memset(buf, blk, len);

	if ( !nbt_buffer_get(nbt_compound_get_child(c->level, "Data"),
				&buf, &len) )
		return 0;
	memset(buf, 0, len);

	if ( !nbt_buffer_get(nbt_compound_get_child(c->level, "HeightMap"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	if ( !nbt_buffer_get(nbt_compound_get_child(c->level, "SkyLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0xff, len);

	if ( !nbt_buffer_get(nbt_compound_get_child(c->level, "BlockLight"),
				&buf, &len) )
		return 0;
	memset(buf, 0x0, len);

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

	ents = nbt_compound_get_child(c->level, "Entities");
	if ( !nbt_list_nuke(ents) )
		rc = 0;

	return rc;
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

	//nbt_dump(c->nbt);
	//printf("decoded %zu bytes of chunk data\n", sz);

	root = nbt_root_tag(c->nbt);
	if ( NULL == root )
		goto out_free;

	c->level = nbt_compound_get_child(root, "Level");
	if ( NULL == c->level )
		goto out_free;

	goto out;

out_free:
	free(c);
	c = NULL;
out:
	return c;
}

void chunk_free(chunk_t c)
{
	if ( c ) {
		nbt_free(c->nbt);
		free(c);
	}
}
