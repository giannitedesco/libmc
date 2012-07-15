/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Handle schematic.dat files
*/
#include <libmc/minecraft.h>
#include <libmc/nbt.h>
#include <libmc/schematic.h>

struct _schematic {
	nbt_t nbt;
	nbt_tag_t schem;
	int16_t x, y, z;
	unsigned ref;
};

schematic_t schematic_load(const char *path)
{
	struct _schematic *s;
	uint8_t *buf;
	size_t sz;

	s = calloc(1, sizeof(*s));
	if ( NULL == s )
		goto out;

	if ( !libmc_gunzip(path, &buf, &sz) )
		goto out_free;

	s->nbt = nbt_decode(buf, sz);
	free(buf);
	if ( NULL == s->nbt )
		goto out_free;

	s->schem = nbt_root_tag(s->nbt);
	if ( NULL == s->schem )
		goto out_free;

	if ( !nbt_short_get(nbt_compound_get(s->schem, "Width"), &s->x) )
		goto out_free;
	if ( !nbt_short_get(nbt_compound_get(s->schem, "Height"), &s->y) )
		goto out_free;
	if ( !nbt_short_get(nbt_compound_get(s->schem, "Length"), &s->z) )
		goto out_free;

	if ( s->x < 0 || s->y < 0 || s->z < 0 )
		goto out_free;

	s->ref = 1;
	goto out;

out_free:
	nbt_free(s->nbt);
	free(s);
	s = NULL;
out:
	return s;
}

static void schematic_free(schematic_t s)
{
	nbt_free(s->nbt);
	free(s);
}

schematic_t schematic_get(schematic_t s)
{
	s->ref++;
	return s;
}

void schematic_put(schematic_t s)
{
	if ( 0 == --s->ref )
		schematic_free(s);
}

void schematic_get_size(schematic_t s, int16_t *x, int16_t *y, int16_t *z)
{
	if ( x )
		*x = s->x;
	if ( y )
		*y = s->y;
	if ( z )
		*z = s->z;
}

uint8_t *schematic_get_blocks(schematic_t s)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_bytearray_get(nbt_compound_get(s->schem, "Blocks"), &buf, &sz) )
		return NULL;

	if ( sz != (size_t)((s->x * s->y * s->z)) ) {
		fprintf(stderr, "schematic: bad blocks size\n");
		return NULL;
	}

	return buf;
}

uint8_t *schematic_get_data(schematic_t s)
{
	uint8_t *buf;
	size_t sz;

	if ( !nbt_bytearray_get(nbt_compound_get(s->schem, "Data"), &buf, &sz) )
		return NULL;

	if ( sz != (size_t)(s->x * s->y * s->z) ) {
		fprintf(stderr, "schematic: bad blocks size\n");
		return NULL;
	}

	return buf;
}

static nbt_tag_t create_schem_keys(struct _schematic *s)
{
	nbt_tag_t root, tag;
	size_t sz = s->x * s->y * s->z;

	root = nbt_root_tag(s->nbt);
	if ( NULL == root )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_Short);
	if ( NULL == tag)
		return NULL;
	nbt_short_set(tag, s->x);
	if ( !nbt_compound_set(root, "Width", tag) )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_Short);
	if ( NULL == tag)
		return NULL;
	nbt_short_set(tag, s->y);
	if ( !nbt_compound_set(root, "Height", tag) )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_Short);
	if ( NULL == tag)
		return NULL;
	nbt_short_set(tag, s->z);
	if ( !nbt_compound_set(root, "Length", tag) )
		return NULL;

	if ( !nbt_compound_set(root, "Entities",
				nbt_tag_new_list(s->nbt, NBT_TAG_Compound)) )
		return NULL;
	if ( !nbt_compound_set(root, "TileEntities",
				nbt_tag_new_list(s->nbt, NBT_TAG_Compound)) )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_String);
	if ( NULL == tag)
		return NULL;
	nbt_string_set(tag, "Alpha");
	if ( !nbt_compound_set(root, "Materials", tag) )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_Byte_Array);
	if ( NULL == tag)
		return NULL;
	nbt_bytearray_set(tag, NULL, sz);
	if ( !nbt_compound_set(root, "Blocks", tag) )
		return NULL;

	tag = nbt_tag_new(s->nbt, NBT_TAG_Byte_Array);
	if ( NULL == tag)
		return NULL;
	nbt_bytearray_set(tag, NULL, sz);
	if ( !nbt_compound_set(root, "Data", tag) )
		return NULL;

	return root;
}

schematic_t schematic_new(int16_t x, int16_t y, int16_t z)
{
	struct _schematic *s;

	assert(x > 0 && y > 0 && z > 0);

	s = calloc(1, sizeof(*s));
	if ( NULL == s )
		goto out;

	s->nbt = nbt_new();
	if ( NULL == s->nbt )
		goto out_free;

	s->x = x;
	s->y = y;
	s->z = z;
	s->schem = create_schem_keys(s);
	if ( NULL == s->schem )
		goto out_free_nbt;

	goto out; /* success */

out_free_nbt:
	nbt_free(s->nbt);
out_free:
	free(s);
	s = NULL;
out:
	return s;
}

schematic_t schematic_dup(schematic_t s, vec3_t mins, vec3_t maxs)
{
	return NULL;
}
