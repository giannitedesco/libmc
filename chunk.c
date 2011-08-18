#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/chunk.h>
#include <libmc/nbt.h>

struct _chunk {
	nbt_t nbt;
	int32_t xpos, zpos;
	uint8_t *blocks;
	uint8_t *data;
};

chunk_t chunk_from_bytes(const uint8_t *buf, size_t sz)
{
	struct _chunk *c;
	nbt_tag_t root, level;
	size_t blen;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->nbt = nbt_decode(buf, sz);
	if ( NULL == c->nbt )
		goto out_free;

	root = nbt_root_tag(c->nbt);
	if ( NULL == root )
		goto out_free;

	level = nbt_compound_get_child(root, "Level");
	if ( NULL == level )
		goto out_free;

	if ( !nbt_int_get(nbt_compound_get_child(level, "xPos"), &c->xpos) )
		goto out_free;
	if ( !nbt_int_get(nbt_compound_get_child(level, "zPos"), &c->zpos) )
		goto out_free;

	if ( !nbt_buffer_get(nbt_compound_get_child(level, "Blocks"),
				&c->blocks, &blen) )
		goto out_free;
	if ( blen != CHUNK_X * CHUNK_Y * CHUNK_Z )
		goto out_free;

	if ( !nbt_buffer_get(nbt_compound_get_child(level, "Data"),
				&c->blocks, &blen) )
		goto out_free;
	if ( blen != (CHUNK_X * CHUNK_Y * CHUNK_Z) / 2)
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
