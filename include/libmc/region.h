#ifndef _REGION_H
#define _REGION_H

#define REGION_X	32
#define REGION_Z	32

typedef struct _region *region_t;

region_t region_open(const char *fn, int rdonly);
region_t region_new(const char *fn);
chunk_t region_get_chunk(region_t r, uint8_t x, uint8_t z);
int region_set_chunk(region_t r, uint8_t x, uint8_t z, chunk_t c);
int region_save(region_t r);
void region_close(region_t r);

#endif /* _REGION_H */
