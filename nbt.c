/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load nbt tag files. This is a TLV format except without the lengths. So
 * we have to load all the data in one shot.
*/
#define _GNU_SOURCE
#include <limits.h>

#include <libmc/minecraft.h>
#include <libmc/nbt.h>

#include "hgang.h"
#include "list.h"

#define TAG_NAMED	0
#define TAG_ANON	1

struct nbt_byte_array {
	int32_t len;
	uint8_t *array;
};

struct nbt_int_array {
	int32_t len;
	int32_t *array;
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
		struct nbt_int_array t_ints;
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
		[NBT_TAG_Int_Array] = "IntArray",
	};
	struct nbt_tag *c;
	int32_t i;

	printf("%*c Tag_%s '%s'",
		2 * depth, ' ', tstr[tag->t_type], tag->t_name);

	switch(tag->t_type) {
	case NBT_TAG_Byte_Array:
		printf(" = %d bytes\n", tag->t_u.t_blob.len);
#if 0
		if ( !strcmp(tag->t_name, "SkyLight") ||
			!strcmp(tag->t_name, "BlockLight") ) {
			hex_dumpf(stdout, tag->t_u.t_blob.array,
					tag->t_u.t_blob.len, 16);
		}
#endif
		break;
	case NBT_TAG_Byte:
		printf(" = %d\n", tag->t_u.t_byte);
		break;
	case NBT_TAG_Short:
		printf(" = %d\n", tag->t_u.t_short);
		break;
	case NBT_TAG_Int:
		printf(" = %"PRId32"\n", tag->t_u.t_int);
		break;
	case NBT_TAG_Long:
		printf(" = %"PRId64"\n", tag->t_u.t_long);
		break;
	case NBT_TAG_Float:
		printf(" = %f\n", tag->t_u.t_float);
		break;
	case NBT_TAG_Double:
		printf(" = %F\n", tag->t_u.t_double);
		break;
	case NBT_TAG_String:
		printf(" = '%s'\n", tag->t_u.t_str);
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
	case NBT_TAG_Int_Array:
		printf(" = %d ints\n", tag->t_u.t_ints.len);
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
	case NBT_TAG_Int_Array:
		if ( ptr + sizeof(alen) > end )
			return NULL;
		alen = be32toh(*(int32_t *)ptr) * sizeof(int32_t);
		aptr =  ptr + sizeof(alen);
		ptr += sizeof(alen) + alen;
		if ( alen < 0 || ptr > end )
			return NULL;
		tag->t_u.t_ints.array = malloc(alen);
		if ( NULL == tag->t_u.t_ints.array )
			return NULL;
		tag->t_u.t_ints.len = alen / sizeof(int32_t);
		memcpy(tag->t_u.t_ints.array, aptr, alen);
		break;
	default:
		printf("nbt: uknown type %d\n", tag->t_type);
		return NULL;
	}

	return ptr;
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
	case NBT_TAG_Int_Array:
		free(tag->t_u.t_ints.array);
		break;
	default:
		break;
	}

	free(tag->t_name);
}

nbt_tag_t nbt_root_tag(nbt_t nbt)
{
	return &nbt->root;
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

int nbt_bytearray_get(nbt_tag_t t, uint8_t **bytes, size_t *sz)
{
	if (NULL == t || t->t_type != NBT_TAG_Byte_Array)
		return 0;
	*bytes = t->t_u.t_blob.array;
	*sz = (size_t)t->t_u.t_blob.len;
	return 1;
}

int nbt_intarray_get(nbt_tag_t t, int32_t **ints, unsigned int *num)
{
	if (NULL == t || t->t_type != NBT_TAG_Int_Array)
		return 0;
	*ints = t->t_u.t_ints.array;
	*num = t->t_u.t_ints.len;
	return 1;
}

int nbt_string_get(nbt_tag_t t, char **val)
{
	if (NULL == t || t->t_type != NBT_TAG_String)
		return 0;
	*val = t->t_u.t_str;
	return 1;
}

nbt_tag_t nbt_list_get(nbt_tag_t t, unsigned idx)
{
	if (NULL == t || t->t_type != NBT_TAG_List)
		return 0;
	if ( idx >= (unsigned)t->t_u.t_list.len )
		return 0;
	return t->t_u.t_list.array[idx];
}

int nbt_list_get_size(nbt_tag_t t)
{
	if (NULL == t || t->t_type != NBT_TAG_List)
		return -1;
	return t->t_u.t_list.len;
}

nbt_tag_t nbt_compound_get(nbt_tag_t t, const char *name)
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

int nbt_byte_set(nbt_tag_t t, uint8_t val)
{
	if ( NULL == t || t->t_type != NBT_TAG_Byte )
		return 0;
	t->t_u.t_byte = val;
	return 1;
}

int nbt_short_set(nbt_tag_t t, int16_t val)
{
	if ( NULL == t || t->t_type != NBT_TAG_Short )
		return 0;
	t->t_u.t_short = val;
	return 1;
}

int nbt_int_set(nbt_tag_t t, int32_t val)
{
	if ( NULL == t || t->t_type != NBT_TAG_Int )
		return 0;
	t->t_u.t_int = val;
	return 1;
}

int nbt_long_set(nbt_tag_t t, int64_t val)
{
	if ( NULL == t || t->t_type != NBT_TAG_Long )
		return 0;
	t->t_u.t_long = val;
	return 1;
}

int nbt_bytearray_set(nbt_tag_t t, const uint8_t *bytes, unsigned int num)
{
	uint8_t *buf;

	if ( NULL == t || t->t_type != NBT_TAG_Byte_Array )
		return 0;

	buf = malloc(num);
	if ( NULL == buf )
		return 0;

	free(t->t_u.t_blob.array);
	memcpy(buf, bytes, num);
	t->t_u.t_blob.array = buf;
	t->t_u.t_blob.len = num;

	return 1;
}

int nbt_intarray_set(nbt_tag_t t, const int32_t *ints, unsigned int num)
{
	int32_t *buf;

	if ( NULL == t || t->t_type != NBT_TAG_Int_Array )
		return 0;

	buf = malloc(sizeof(int32_t) * num);
	if ( NULL == buf )
		return 0;

	free(t->t_u.t_ints.array);
	memcpy(buf, ints, num);
	t->t_u.t_ints.array = buf;
	t->t_u.t_ints.len = num;

	return 1;
}

int nbt_string_set(nbt_tag_t t, const char *val)
{
	char *str;

	if ( NULL == t || t->t_type != NBT_TAG_String )
		return 0;

	str = strdup(val);
	if ( NULL == str )
		return 0;

	free(t->t_u.t_str);
	t->t_u.t_str = str;

	return 1;
}

int nbt_list_set(nbt_tag_t t, unsigned idx, nbt_tag_t val)
{
	if ( NULL == t || t->t_type != NBT_TAG_List )
		return 0;
	if ( val->t_type != t->t_u.t_list.type )
		return 0;
	if ( idx > INT_MAX || (unsigned)t->t_u.t_list.len <= idx )
		return 0;
	t->t_u.t_list.array[idx] = val;
	return 1;

}

int nbt_list_set_size(nbt_tag_t t, unsigned sz)
{
	nbt_tag_t *new;

	if ( NULL == t || t->t_type != NBT_TAG_List )
		return 0;

	new = realloc(t->t_u.t_list.array, sz * sizeof(*new));
	if ( NULL == new )
		return 0;

	t->t_u.t_list.array = new;
	t->t_u.t_list.len = sz;
	return 1;
}

int nbt_list_append(nbt_tag_t t, nbt_tag_t val)
{
	unsigned int idx = t->t_u.t_list.len;
	if ( !nbt_list_set_size(t, t->t_u.t_list.len + 1) )
		return 0;
	if ( !nbt_list_set(t, idx, val) )
		return 0;
	return 1;
}

int nbt_compound_delete(nbt_tag_t t, const char *key)
{
	struct nbt_tag *c;

	if ( NULL == t || t->t_type != NBT_TAG_Compound )
		return 0;

	list_for_each_entry(c, &t->t_u.t_compound, t_list) {
		if ( !strcmp(c->t_name, key) ) {
			list_del(&t->t_list);
			free_nbt_data(c);
			return 1;
		}
	}

	return 1;
}

int nbt_compound_set(nbt_tag_t t, const char *key, nbt_tag_t val)
{
	char *name;

	if ( NULL == t || t->t_type != NBT_TAG_Compound )
		return 0;

	name = strdup(key);
	if ( NULL == name )
		return 0;

	free(val->t_name);
	val->t_name = name;
	nbt_compound_delete(t, key);
	list_add_tail(&val->t_list, &t->t_u.t_compound);
	return 1;
}


int nbt_list_nuke(nbt_tag_t t)
{
	int32_t i;

	if (NULL == t || t->t_type != NBT_TAG_List)
		return 0;

	for(i = 0; i < t->t_u.t_list.len; i++)
		free_nbt_data(t->t_u.t_list.array[i]);
	free(t->t_u.t_list.array);

	t->t_u.t_list.len = 0;
	t->t_u.t_list.array = NULL;
	return 1;
}

int nbt_compound_nuke(nbt_tag_t t)
{
	if (NULL == t || t->t_type != NBT_TAG_Compound)
		return 0;

	/* TODO: compound nuke */
	return 1;
}

static struct nbt_tag *new_node(struct _nbt *nbt, uint8_t type,
				uint8_t list_type)
{
	struct nbt_tag *tag;

	if ( type >= NBT_TAG_MAX )
		return NULL;

	/* use nbt_tag_new_list instead */
	if ( type == NBT_TAG_List && list_type == NBT_TAG_End )
		return NULL;

	tag = hgang_alloc0(nbt->nodes);
	if ( NULL == tag )
		return NULL;

	tag->t_type = type;

	/* type specific initialisation */
	switch(tag->t_type) {
	case NBT_TAG_Compound:
		INIT_LIST_HEAD(&tag->t_u.t_compound);
	case NBT_TAG_List:
		tag->t_u.t_list.type = list_type;
		break;
	default:
		break;
	}

	return tag;
}

nbt_tag_t nbt_tag_new(nbt_t nbt, uint8_t type)
{
	return new_node(nbt, type, NBT_TAG_End);
}

nbt_tag_t nbt_tag_new_list(nbt_t nbt, uint8_t type)
{
	return new_node(nbt, NBT_TAG_List, type);
}

char *nbt_tag_name(nbt_tag_t t)
{
	if ( NULL == t )
		return NULL;
	return t->t_name;
}

static void do_get_size(struct nbt_tag *tag, int type, size_t *sz)
{
	struct nbt_tag *c;
	int32_t i;

	if ( type == TAG_NAMED ) {
		*sz += 3 + strlen(tag->t_name);
	}

	switch(tag->t_type) {
	case NBT_TAG_Byte:
		*sz += sizeof(uint8_t);
		break;
	case NBT_TAG_Short:
		*sz += sizeof(int16_t);
		break;
	case NBT_TAG_Int:
		*sz += sizeof(int32_t);
		break;
	case NBT_TAG_Long:
		*sz += sizeof(int64_t);
		break;
	case NBT_TAG_Float:
		*sz += sizeof(float);
		break;
	case NBT_TAG_Double:
		*sz += sizeof(double);
		break;
	case NBT_TAG_Byte_Array:
		*sz += sizeof(tag->t_u.t_blob.len) + tag->t_u.t_blob.len;
		break;
	case NBT_TAG_String:
		*sz += sizeof(int16_t) + strlen(tag->t_u.t_str);
		break;
	case NBT_TAG_List:
		*sz += sizeof(uint8_t) + sizeof(int32_t);
		for(i = 0; i < tag->t_u.t_list.len; i++)
			do_get_size(tag->t_u.t_list.array[i], TAG_ANON, sz);
		break;
	case NBT_TAG_Compound:
		list_for_each_entry(c, &tag->t_u.t_compound, t_list)
			do_get_size(c, TAG_NAMED, sz);
		*sz += 1;
		break;
	case NBT_TAG_Int_Array:
		*sz += sizeof(tag->t_u.t_ints.len) +
			(tag->t_u.t_ints.len * sizeof(int32_t));
		break;
	default:
		break;
	}
}

size_t nbt_size_in_bytes(nbt_t nbt)
{
	size_t sz = 0;
	do_get_size(&nbt->root, TAG_NAMED, &sz);
	return sz;
}

static int do_get_bytes(struct nbt_tag *tag, int type,
				uint8_t **pptr, uint8_t *end)
{
	uint8_t *ptr = *pptr;
	struct nbt_tag *c;
	int16_t slen;
	int32_t i;

	if ( type == TAG_NAMED ) {
		slen = strlen(tag->t_name);

		if ( ptr + 3 + slen > end )
			return 0;

		*ptr = tag->t_type;
		ptr++;

		*(int16_t *)ptr = htobe16(slen);
		ptr += sizeof(int16_t);

		memcpy(ptr, tag->t_name, slen);
		ptr += slen;
	}

	switch(tag->t_type) {
	case NBT_TAG_Byte:
		if ( ptr + sizeof(uint8_t) > end )
			return 0;
		*ptr = tag->t_u.t_byte;
		ptr += sizeof(uint8_t);
		break;
	case NBT_TAG_Short:
		if ( ptr + sizeof(uint16_t) > end )
			return 0;
		*(int16_t *)ptr = htobe16(tag->t_u.t_short);
		ptr += sizeof(int16_t);
		break;
	case NBT_TAG_Int:
		if ( ptr + sizeof(uint32_t) > end )
			return 0;
		*(int32_t *)ptr = htobe32(tag->t_u.t_int);
		ptr += sizeof(int32_t);
		break;
	case NBT_TAG_Long:
		if ( ptr + sizeof(uint64_t) > end )
			return 0;
		*(int64_t *)ptr = htobe64(tag->t_u.t_long);
		ptr += sizeof(int64_t);
		break;
	case NBT_TAG_Float:
		if ( ptr + sizeof(float) > end )
			return 0;
		*(float *)ptr = htobe32((int32_t)tag->t_u.t_float);
		ptr += sizeof(float);
		break;
	case NBT_TAG_Double:
		if ( ptr + sizeof(double) > end )
			return 0;
		*(double *)ptr = htobe64((int64_t)tag->t_u.t_double);
		ptr += sizeof(double);
		break;
	case NBT_TAG_Byte_Array:
		if ( ptr + sizeof(int32_t) + tag->t_u.t_blob.len > end ) {
			return 0;
		}
		*(int32_t *)ptr = htobe32(tag->t_u.t_blob.len);
		ptr += sizeof(int32_t);
		memcpy(ptr, tag->t_u.t_blob.array, tag->t_u.t_blob.len);
		ptr += tag->t_u.t_blob.len;
		break;
	case NBT_TAG_String:
		slen = strlen(tag->t_u.t_str);
		if ( ptr + sizeof(int16_t) + slen > end )
			return 0;
		*(int16_t *)ptr = htobe16(slen);
		memcpy(ptr + sizeof(int16_t), tag->t_u.t_str, slen);
		ptr += sizeof(int16_t) + slen;
		break;
	case NBT_TAG_List:
		if ( ptr +  sizeof(uint8_t) + sizeof(int32_t) > end )
			return 0;
		*ptr = tag->t_u.t_list.type;
		ptr += sizeof(uint8_t);
		*(int32_t *)ptr = htobe32(tag->t_u.t_list.len);
		ptr += sizeof(int32_t);

		for(i = 0; i < tag->t_u.t_list.len; i++)
			if ( !do_get_bytes(tag->t_u.t_list.array[i],
					TAG_ANON, &ptr, end) )
				return 0;
		break;
	case NBT_TAG_Compound:
		list_for_each_entry(c, &tag->t_u.t_compound, t_list)
			if ( !do_get_bytes(c, TAG_NAMED, &ptr, end) )
				return 0;

		if ( ptr + sizeof(uint8_t) > end )
			return 0;
		*ptr = NBT_TAG_End;
		ptr += sizeof(uint8_t);
		break;
	case NBT_TAG_Int_Array:
		if ( ptr + sizeof(int32_t) +
			(tag->t_u.t_ints.len * sizeof(int32_t)) > end ) {
			return 0;
		}
		*(int32_t *)ptr = htobe32(tag->t_u.t_ints.len);
		ptr += sizeof(int32_t);
		memcpy(ptr, tag->t_u.t_ints.array,
			tag->t_u.t_ints.len * sizeof(int32_t));
		ptr += tag->t_u.t_ints.len * sizeof(int32_t);
		break;
	default:
		return 0;
	}

	*pptr = ptr;
	return 1;
}

int nbt_get_bytes(nbt_t nbt, uint8_t *buf, size_t len)
{
	uint8_t **pptr = &buf;
	return do_get_bytes(&nbt->root, TAG_NAMED, pptr, buf + len);
}

static struct _nbt *create_nbt(void)
{
	struct _nbt *nbt;

	nbt = calloc(1, sizeof(*nbt));
	if ( NULL == nbt )
		goto out;

	nbt->nodes = hgang_new(sizeof(struct nbt_tag), 0);
	if ( NULL == nbt->nodes )
		goto out_free;

	goto out;

out_free:
	free(nbt);
	nbt = NULL;
out:
	return nbt;
}

nbt_t nbt_decode(const uint8_t *buf, size_t len)
{
	const uint8_t *ptr;
	struct _nbt *nbt;

	nbt = create_nbt();
	if ( NULL == nbt )
		return NULL;

	ptr = decode_tag(nbt, buf, len, &nbt->root, TAG_NAMED);
	if ( NULL == ptr ) {
		nbt_free(nbt);
		return NULL;
	}

	return nbt;
}

nbt_t nbt_new(void)
{
	struct _nbt *nbt;

	nbt = create_nbt();
	if ( NULL == nbt )
		return NULL;

	nbt->root.t_type = NBT_TAG_Compound;
	nbt->root.t_name = strdup("");
	if ( NULL == nbt->root.t_name ) {
		nbt_free(nbt);
		return NULL;
	}
	INIT_LIST_HEAD(&nbt->root.t_u.t_compound);
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
