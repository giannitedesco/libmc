#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/chunk.h>
#include <libmc/nbt.h>

struct _chunk {
	int foo;
};

chunk_t chunk_from_bytes(const uint8_t *buf, size_t sz)
{
	struct _chunk *c;
	nbt_t n;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	n = nbt_decode(buf, sz);
	if ( NULL == n )
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
	free(c);
}
