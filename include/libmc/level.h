#ifndef _LEVEL_H
#define _LEVEL_H

typedef struct _level *level_t;

level_t level_load(const char *path);
level_t level_new(void);
level_t level_get(level_t l);
void level_put(level_t l);

#endif /* _LEVEL_H */
