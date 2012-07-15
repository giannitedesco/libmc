#ifndef _MINECRAFT_H
#define _MINECRAFT_H

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

typedef int scalar_t;
typedef scalar_t vec2_t[2];
typedef scalar_t vec3_t[3];

static inline int s_min(int a, int b)
{
	return (a < b) ? a : b;
}

static inline int s_max(int a, int b)
{
	return (a > b) ? a : b;
}

#endif /* _MINECRAFT_H */
