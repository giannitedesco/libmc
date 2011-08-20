/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load nbt tag files. This is a TLV format except without the lengths. So
 * we have to load all the data in one shot.
*/
#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/nbt.h>

#include "hgang.h"
#include "list.h"

#define TAG_NAMED	0
#define TAG_ANON	1

struct nbt_byte_array {
	int32_t len;
	uint8_t *array;
};

struct nbt_list {
	struct nbt_tag **array;
	int32_t len;
	uint8_t type;
};

struct nbt_tag {
	struct list_head t_list;
	char *t_name;
	union {
		uint8_t t_byte;
		int16_t t_short;
		int32_t t_int;
		int64_t t_long;
		float t_float;
		double t_double;
		struct nbt_byte_array t_blob;
		char *t_str;
		struct nbt_list t_list;
		struct list_head t_compound;
	}t_u;
	uint8_t t_type;
};

struct _nbt {
	hgang_t nodes;
	struct nbt_tag root;
};

#if 0
static void hex_dumpf(FILE *f, const uint8_t *tmp, size_t len, size_t llen)
{
	size_t i, j;
	size_t line;

	if ( NULL == f || 0 == len )
		return;

	for(j = 0; j < len; j += line, tmp += line) {
		if ( j + llen > len ) {
			line = len - j;
		}else{
			line = llen;
		}

		fprintf(f, "%05x : ", j);

		for(i = 0; i < line; i++) {
			if ( isprint(tmp[i]) ) {
				fprintf(f, "%c", tmp[i]);
			}else{
				fprintf(f, ".");
			}
		}

		for(; i < llen; i++)
			fprintf(f, " ");

		for(i = 0; i < line; i++)
			fprintf(f, " %02x", tmp[i]);

		fprintf(f, "\n");
	}
	fprintf(f, "\n");
}
#endif

static void do_dump(struct nbt_tag *tag, unsigned int depth)
{
	static const char * const tstr[] = {
		[NBT_TAG_End] = "End",
		[NBT_TAG_Byte] = "Byte",
		[NBT_TAG_Short] = "Short",
		[NBT_TAG_Int] = "Int",
		[NBT_TAG_Long] = "Long",
		[NBT_TAG_Float] = "Float",
		[NBT_TAG_Double] = "Double",
		[NBT_TAG_Byte_Array] = "ByteArray",
		[NBT_TAG_String] = "String",
		[NBT_TAG_List] = "List",
		[NBT_TAG_Compound] = "Compound",
	};
	struct nbt_tag *c;
	int32_t i;

	printf("%*c Tag_%s: %s",
		2 * depth, ' ', tstr[tag->t_type], tag->t_name);

	switch(tag->t_type) {
	case NBT_TAG_Byte_Array:
		printf(" == %d bytes\n", tag->t_u.t_blob.len);
		break;
	case NBT_TAG_Byte:
		printf(" == %d\n", tag->t_u.t_byte);
		break;
	case NBT_TAG_Short:
		printf(" == %d\n", tag->t_u.t_short);
		break;
	case NBT_TAG_Int:
		printf(" == %"PRId32"\n", tag->t_u.t_int);
		break;
	case NBT_TAG_Long:
		printf(" == %"PRId64"\n", tag->t_u.t_long);
		break;
	case NBT_TAG_Float:
		printf(" == %f\n", tag->t_u.t_float);
		break;
	case NBT_TAG_Double:
		printf(" == %F\n", tag->t_u.t_double);
		break;
	case NBT_TAG_String:
		printf(" == '%s'\n", tag->t_u.t_str);
		break;
	case NBT_TAG_List:
		printf(" type = %s [%d] = {\n",
			tstr[tag->t_u.t_list.type],
			tag->t_u.t_list.len);
		for(i = 0; i < tag->t_u.t_list.len; i++)
			do_dump(tag->t_u.t_list.array[i], depth + 1);
		printf("%*c }\n", depth * 2, ' ');
		break;
	case NBT_TAG_Compound:
		printf(" {\n");
		list_for_each_entry(c, &tag->t_u.t_compound, t_list)
			do_dump(c, depth + 1);
		printf("%*c }\n", depth * 2, ' ');
		break;
	default:
		printf("\n");
		break;
	}
}

void nbt_dump(nbt_t nbt)
{
	do_dump(&nbt->root, 0);
}

static const uint8_t *decode_tag(struct _nbt *nbt,
					const uint8_t *ptr, size_t len,
					struct nbt_tag *tag, int type)
{
	const uint8_t *end = ptr + len, *tmp;
	const uint8_t *aptr;
	struct nbt_tag *c;
	int32_t cnt, alen;
	int16_t slen;
	char *str;

	if ( type == TAG_NAMED ) {
		if ( len < 1 )
			return NULL;

		tag->t_type = *ptr;
		ptr++;

		if ( tag->t_type != NBT_TAG_End ) {
			if ( ptr + 2 > end )
				return NULL;
			slen = be16toh(*(int16_t *)ptr);
			str = (char *)(ptr + 2);
			ptr += 2 + slen;
			if ( slen < 0 || ptr > end )
				return NULL;
			if ( asprintf(&tag->t_name, "%.*s", slen, str) < 0 )
				return NULL;
		}else{
			tag->t_name = NULL;
		}
	}

	switch(tag->t_type) {
	case NBT_TAG_End:
		break;
	case NBT_TAG_Byte:
		if ( ptr + sizeof(tag->t_u.t_byte) > end )
			return NULL;
		tag->t_u.t_byte = *ptr;
		ptr += sizeof(tag->t_u.t_byte);
		break;
	case NBT_TAG_Short:
		if ( ptr + sizeof(tag->t_u.t_short) > end )
			return NULL;
		tag->t_u.t_short = be16toh(*(int16_t *)ptr);
		ptr += sizeof(tag->t_u.t_short);
		break;
	case NBT_TAG_Int:
		if ( ptr + sizeof(tag->t_u.t_int) > end )
			return NULL;
		tag->t_u.t_int = be32toh(*(int32_t *)ptr);
		ptr += sizeof(tag->t_u.t_int);
		break;
	case NBT_TAG_Long:
		if ( ptr + sizeof(tag->t_u.t_long) > end )
			return NULL;
		tag->t_u.t_long = be64toh(*(int64_t *)ptr);
		ptr += sizeof(tag->t_u.t_long);
		break;
	case NBT_TAG_Float:
		if ( ptr + sizeof(tag->t_u.t_float) > end )
			return NULL;
		tag->t_u.t_float = (float)be32toh(*(int32_t *)ptr);
		ptr += sizeof(tag->t_u.t_float);
		break;
	case NBT_TAG_Double:
		if ( ptr + sizeof(tag->t_u.t_double) > end )
			return NULL;
		tag->t_u.t_double = (double)be64toh(*(int64_t *)ptr);
		ptr += sizeof(tag->t_u.t_double);
		break;
	case NBT_TAG_Byte_Array:
		if ( ptr + sizeof(alen) > end )
			return NULL;
		alen = be32toh(*(int32_t *)ptr);
		aptr =  ptr + sizeof(alen);
		ptr += sizeof(alen) + alen;
		if ( alen < 0 || ptr > end )
			return NULL;
		tag->t_u.t_blob.array = malloc(alen);
		if ( NULL == tag->t_u.t_blob.array )
			return NULL;
		tag->t_u.t_blob.len = alen;
		memcpy(tag->t_u.t_blob.array, aptr, alen);
		break;
	case NBT_TAG_String:
		if ( ptr + sizeof(tag->t_u.t_short) > end )
			return NULL;
		slen = be16toh(*(int16_t *)ptr);
		str = (char *)ptr + 2;
		ptr += sizeof(tag->t_u.t_short) + slen;
		if ( slen < 0 || ptr > end )
			return NULL;
		if ( asprintf(&tag->t_u.t_str, "%.*s", slen, str) < 0 )
			return NULL;
		break;
	case NBT_TAG_List:
		if ( ptr + sizeof(uint8_t) + sizeof(int32_t) > end )
			return NULL;

		tag->t_u.t_list.type = *ptr;
		ptr++;
		if ( tag->t_u.t_list.type == NBT_TAG_End ) {
			printf("Bad list format\n");
			return NULL;
		}

		tag->t_u.t_list.len = be32toh(*(int32_t *)ptr);
		ptr += 4;

		tag->t_u.t_list.array = calloc(tag->t_u.t_list.len,
						sizeof(*tag->t_u.t_list.array));
		if ( NULL == tag->t_u.t_list.array )
			return NULL;

		for(cnt = 0; cnt < tag->t_u.t_list.len; cnt++) {
			c = hgang_alloc0(nbt->nodes);
			if ( NULL == c )
				return NULL;
			tag->t_u.t_list.array[cnt] = c;
			c->t_type = tag->t_u.t_list.type;
			tmp = decode_tag(nbt, ptr, end - ptr, c, TAG_ANON);
			if ( NULL == tmp )
				return NULL;
			ptr = tmp;
		}
		break;
	case NBT_TAG_Compound:
		INIT_LIST_HEAD(&tag->t_u.t_compound);
		while(ptr < end) {
			c = hgang_alloc0(nbt->nodes);
			if ( NULL == c )
				return NULL;
			tmp = decode_tag(nbt, ptr, end - ptr, c, TAG_NAMED);
			if ( NULL == tmp )
				return NULL;
			ptr = tmp;
			if ( c->t_type == NBT_TAG_End ) {
				hgang_return(nbt->nodes, c);
				break;
			}else{
				list_add_tail(&c->t_list, &tag->t_u.t_compound);
			}
		}
		break;
	default:
		printf("nbt: uknown type %d\n", tag->t_type);
		return NULL;
	}

	return ptr;
}

nbt_tag_t nbt_root_tag(nbt_t nbt)
{
	return &nbt->root;
}

nbt_tag_t nbt_compound_get_child(nbt_tag_t t, const char *name)
{
	struct nbt_tag *c;

	if (NULL == t || t->t_type != NBT_TAG_Compound)
		return NULL;

	list_for_each_entry(c, &t->t_u.t_compound, t_list) {
		if ( !strcmp(c->t_name, name) ) {
			return c;
		}
	}

	return NULL;
}

int nbt_byte_get(nbt_tag_t t, uint8_t *val)
{
	if (NULL == t || t->t_type != NBT_TAG_Byte)
		return 0;
	*val = t->t_u.t_byte;
	return 1;
}

int nbt_short_get(nbt_tag_t t, int16_t *val)
{
	if (NULL == t || t->t_type != NBT_TAG_Short)
		return 0;
	*val = t->t_u.t_short;
	return 1;
}

int nbt_int_get(nbt_tag_t t, int32_t *val)
{
	if (NULL == t || t->t_type != NBT_TAG_Int)
		return 0;
	*val = t->t_u.t_int;
	return 1;
}

int nbt_long_get(nbt_tag_t t, int64_t *val)
{
	if (NULL == t || t->t_type != NBT_TAG_Long)
		return 0;
	*val = t->t_u.t_long;
	return 1;
}

int nbt_buffer_get(nbt_tag_t t, const uint8_t **bytes, size_t *sz)
{
	if (NULL == t || t->t_type != NBT_TAG_Byte_Array)
		return 0;
	*bytes = t->t_u.t_blob.array;
	*sz = (size_t)t->t_u.t_blob.len;
	return 1;
}

int nbt_string_get(nbt_tag_t t, char **val)
{
	if (NULL == t || t->t_type != NBT_TAG_String)
		return 0;
	*val = t->t_u.t_str;
	return 1;
}

int nbt_list_get(nbt_tag_t t, unsigned idx, nbt_tag_t *val)
{
	if (NULL == t || t->t_type != NBT_TAG_List)
		return 0;
	if ( idx >= (unsigned)t->t_u.t_list.len )
		return 0;
	*val = t->t_u.t_list.array[idx];
	return 1;
}

int nbt_list_size(nbt_tag_t t)
{
	if (NULL == t || t->t_type != NBT_TAG_List)
		return -1;
	return t->t_u.t_list.len;
}

char *nbt_tag_name(nbt_tag_t t)
{
	if ( NULL == t )
		return NULL;
	return t->t_name;
}

static void free_nbt_data(struct nbt_tag *tag)
{
	struct nbt_tag *c;
	int32_t i;

	switch(tag->t_type) {
	case NBT_TAG_Byte_Array:
		free(tag->t_u.t_blob.array);
		break;
	case NBT_TAG_String:
		free(tag->t_u.t_str);
		break;
	case NBT_TAG_List:
		for(i = 0; i < tag->t_u.t_list.len; i++)
			free_nbt_data(tag->t_u.t_list.array[i]);
		free(tag->t_u.t_list.array);
		break;
	case NBT_TAG_Compound:
		list_for_each_entry(c, &tag->t_u.t_compound, t_list)
			free_nbt_data(c);
		break;
	default:
		break;
	}

	free(tag->t_name);
}

nbt_t nbt_decode(const uint8_t *buf, size_t len)
{
	const uint8_t *ptr;
	struct _nbt *nbt;

	nbt = calloc(1, sizeof(*nbt));
	if ( NULL == nbt )
		goto out;

	nbt->nodes = hgang_new(sizeof(struct nbt_tag), 0);
	if ( NULL == nbt->nodes )
		goto out_free;

	ptr = decode_tag(nbt, buf, len, &nbt->root, TAG_NAMED);
	if ( NULL == ptr )
		goto out_free_all;

	goto out;

out_free_all:
	free_nbt_data(&nbt->root);
	hgang_free(nbt->nodes);
out_free:
	free(nbt);
	nbt = NULL;
out:
	return nbt;
}

void nbt_free(nbt_t nbt)
{
	if ( nbt ) {
		free_nbt_data(&nbt->root);
		hgang_free(nbt->nodes);
		free(nbt);
	}
}
