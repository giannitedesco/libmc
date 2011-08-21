#ifndef _CHUNK_H
#define _CHUNK_H

#define CHUNK_X 16
#define CHUNK_Y 128
#define CHUNK_Z 16

#define CHUNK_ENC_RAW	0
#define CHUNK_ENC_ZLIB	1

typedef struct _chunk *chunk_t;

chunk_t chunk_from_bytes(uint8_t *buf, size_t sz);
uint8_t *chunk_encode(chunk_t c, int enc, size_t *sz);
void chunk_free(chunk_t c);

int chunk_solid(chunk_t c, unsigned int blk);

#endif /* _CHUNK_H */
