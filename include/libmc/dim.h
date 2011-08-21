#ifndef _DIM_H
#define _DIM_H

typedef struct _dim *dim_t;

dim_t dim_open(const char *dir, int rdonly);
region_t dim_get_region(dim_t d, int x, int z);
void dim_close(dim_t d);

#endif /* _DIM_H */
