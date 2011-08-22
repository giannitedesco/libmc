#ifndef _CHUNK_H
#define _CHUNK_H

#define CHUNK_X 16
#define CHUNK_Y 128
#define CHUNK_Z 16

#define CHUNK_ENC_RAW	0
#define CHUNK_ENC_ZLIB	1

typedef struct _chunk *chunk_t;

chunk_t chunk_from_bytes(uint8_t *buf, size_t sz);
chunk_t chunk_new(void);
int chunk_set_pos(chunk_t c, uint8_t x, uint8_t z);
uint8_t *chunk_encode(chunk_t c, int enc, size_t *sz);
int chunk_strip_entities(chunk_t c);
chunk_t chunk_get(chunk_t c);
void chunk_put(chunk_t c);

int chunk_solid(chunk_t c, unsigned int blk);

#endif /* _CHUNK_H */
