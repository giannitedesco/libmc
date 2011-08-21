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

#include <libmc/chunk.h>
#include <libmc/nbt.h>

struct _chunk {
	nbt_t nbt;
	nbt_tag_t level;
};

size_t chunk_size_in_bytes(chunk_t c)
{
	return nbt_size_in_bytes(c->nbt);
}

chunk_t chunk_from_bytes(uint8_t *buf, size_t sz)
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

	nbt_dump(c->nbt);
	printf("decoded %zu bytes of chunk data\n", sz);

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
