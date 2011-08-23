#ifndef _WORLD_H
#define _WORLD_H

typedef struct _world *world_t;

world_t world_open(const char *dir);
world_t world_create(void);
dim_t world_get_nether(world_t w);
dim_t world_get_earth(world_t w);
level_t world_get_level(world_t w);
int world_save(world_t w, const char *dir);
void world_close(world_t w);

#endif /* _WORLD_H */
