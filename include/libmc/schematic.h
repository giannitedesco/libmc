#ifndef _SCHEMATIC_H
#define _SCHEMATIC_H

typedef struct _schematic *schematic_t;

schematic_t schematic_load(const char *path);
schematic_t schematic_get(schematic_t s);
void schematic_put(schematic_t s);

void schematic_get_size(schematic_t s, int16_t *x, int16_t *y, int16_t *z);
uint8_t *schematic_get_blocks(schematic_t s);
uint8_t *schematic_get_data(schematic_t s);

#endif /* _SCHEMATIC_H */
