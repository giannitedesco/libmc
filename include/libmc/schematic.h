#ifndef _SCHEMATIC_H
#define _SCHEMATIC_H

typedef struct _schematic *schematic_t;

schematic_t schematic_load(const char *path);
schematic_t schematic_get(schematic_t l);
void schematic_put(schematic_t l);

#endif /* _SCHEMATIC_H */
