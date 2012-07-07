#ifndef _REGION_H
#define _REGION_H

#define REGION_X	32
#define REGION_Z	32

typedef struct _region *region_t;

region_t region_open(const char *fn);
region_t region_new(const char *fn);

void region_set_pos(region_t r, int32_t x, int32_t z);

/* load chunk and return reference */
chunk_t region_get_chunk(region_t r, uint8_t x, uint8_t z);

/* set updated chunk, marks chunk as dirty and to be written out,
 * chunk refcount is incremented
*/
int region_set_chunk(region_t r, uint8_t x, uint8_t z, chunk_t c);

void region_set_timestamp(region_t r, uint8_t x, uint8_t z, uint32_t ts);

uint32_t region_get_timestamp(region_t r, uint8_t x, uint8_t z);

int region_paste_schematic(region_t r, schematic_t s, int x, int y, int z);

/* dirty chunks reference counts are dropped */
int region_save(region_t r);

region_t region_get(region_t r);
void region_put(region_t r);

#endif /* _REGION_H */
