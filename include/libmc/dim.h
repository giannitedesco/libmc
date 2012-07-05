#ifndef _DIM_H
#define _DIM_H

typedef struct _dim *dim_t;

dim_t dim_open(const char *dir);
region_t dim_get_region(dim_t d, int x, int z);
region_t dim_new_region(dim_t d, int x, int z);
dim_t dim_create(const char *dir);
void dim_close(dim_t d);

#endif /* _DIM_H */
