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

int chunk_set_pos(chunk_t c, int32_t x, int32_t  z);
int chunk_set_terrain_populated(chunk_t c, uint32_t p);

chunk_t chunk_get(chunk_t c);
void chunk_put(chunk_t c);

uint8_t *chunk_encode(chunk_t c, int enc, size_t *sz);

/* get blocks/data */
uint8_t *chunk_get_blocks(chunk_t c);
uint8_t *chunk_get_data(chunk_t c);

/* higher-level operations */
int chunk_strip_entities(chunk_t c);
int chunk_solid(chunk_t c, unsigned int blk);
int chunk_floor(chunk_t c, uint8_t y, unsigned int blk);

/* TODO: proper re-lighting */

#endif /* _CHUNK_H */
