#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/chunk.h>
#include <libmc/nbt.h>

struct _chunk {
	struct nbt_tag root;
};

chunk_t chunk_from_bytes(const uint8_t *buf, size_t sz)
{
	const uint8_t *ptr, *end;
	struct _chunk *c;
	struct nbt_tag t;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	end = buf + sz;

	ptr = nbt_decode(buf, sz, &t);
	if ( NULL == ptr )
		goto out_free;

	if ( t.t_type != NBT_TAG_Compound )
		goto out_free;

	ptr = nbt_decode(ptr, end - ptr, &c->root);
	if ( NULL == ptr )
		goto out_free;

	printf("decoded chunk '%.*s'\n",
		c->root.t_name.len, c->root.t_name.str);

	goto out;

out_free:
	free(c);
	c = NULL;
out:
	return c;
}

void chunk_free(chunk_t c)
{
	free(c);
}
