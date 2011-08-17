#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/chunk.h>

struct _chunk {
};

chunk_t chunk_from_bytes(const uint8_t *buf, size_t sz)
{
	return NULL;
}

void chunk_free(chunk_t c)
{
	free(c);
}
