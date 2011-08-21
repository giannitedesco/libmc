#ifndef _CHUNK_H
#define _CHUNK_H

#define CHUNK_X 16
#define CHUNK_Y 128
#define CHUNK_Z 16

typedef struct _chunk *chunk_t;

chunk_t chunk_from_bytes(uint8_t *buf, size_t sz);
size_t chunk_size_in_bytes(chunk_t c);
void chunk_free(chunk_t c);

#endif /* _CHUNK_H */
