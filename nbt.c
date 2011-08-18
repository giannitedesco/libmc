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

struct nbt_string {
	int len;
	const char *str;
};

struct nbt_byte_array {
	int32_t len;
	const uint8_t *array;
};

struct nbt_list {
	uint8_t type;
	int32_t len;
	struct list_head list;
};

struct nbt_compound {
	struct list_head list;
};

struct nbt_tag {
	struct nbt_string t_name;
	uint8_t t_type;
	union {
		uint8_t t_byte;
		int16_t t_short;
		int32_t t_int;
		int64_t t_long;
		float t_float;
		double t_double;
		struct nbt_byte_array t_blob;
		struct nbt_string t_str;
		struct nbt_list t_list;
		struct nbt_compound t_compound;
	}t_u;
	struct list_head t_list;
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

static const uint8_t *decode_tag(struct _nbt *nbt,
					const uint8_t *ptr, size_t len,
					struct nbt_tag *tag, int type,
					unsigned int depth)
{
	const uint8_t *end = ptr + len, *tmp;
	struct nbt_tag *c;
	int32_t cnt;

	if ( type == TAG_NAMED ) {
		if ( len < 1 )
			return NULL;

		tag->t_type = *ptr;
		ptr++;

		if ( tag->t_type != NBT_TAG_End ) {
			if ( ptr + 2 > end )
				return NULL;
			tag->t_name.len = be16toh(*(int16_t *)ptr);
			tag->t_name.str = (char *)(ptr + 2);
			ptr += 2 + tag->t_name.len;
			if ( tag->t_name.len < 0 || ptr > end )
				return NULL;
		}else{
			tag->t_name.len = 0;
			tag->t_name.str = NULL;
		}

		//printf("%*c Tag: %.*s\n",
		//	depth, ' ', tag->t_name.len, tag->t_name.str);
	}else{
		//printf("%*c List item\n", depth, ' ');
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
		if ( ptr + 4 > end )
			return NULL;
		tag->t_u.t_blob.len = be32toh(*(int32_t *)ptr);
		tag->t_u.t_blob.array = ptr + 4;
		ptr += 4 + tag->t_u.t_blob.len;
		if ( tag->t_u.t_blob.len < 0 || ptr > end )
			return NULL;
		break;
	case NBT_TAG_String:
		if ( ptr + sizeof(tag->t_u.t_short) > end )
			return NULL;
		tag->t_u.t_str.len = be16toh(*(int16_t *)ptr);
		ptr += sizeof(tag->t_u.t_short);
		tag->t_u.t_str.str = (char *)ptr;
		ptr += tag->t_u.t_str.len;
		break;
	case NBT_TAG_List:
		if ( ptr + 5 > end )
			return NULL;
		tag->t_u.t_list.type = *ptr;
		ptr++;
		tag->t_u.t_list.len = be32toh(*(int32_t *)ptr);
		ptr += 4;

		if ( tag->t_u.t_list.type == NBT_TAG_End ) {
			printf("Bad list format\n");
			return NULL;
		}

		INIT_LIST_HEAD(&tag->t_u.t_list.list);
		for(cnt = 0; cnt < tag->t_u.t_list.len; cnt++) {
			c = hgang_alloc0(nbt->nodes);
			if ( NULL == c )
				return NULL;
			c->t_type = tag->t_u.t_list.type;
			tmp = decode_tag(nbt, ptr, end - ptr, c, TAG_ANON,
					depth + 1);
			if ( NULL == tmp )
				return NULL;
			ptr = tmp;
			list_add_tail(&c->t_list, &tag->t_u.t_list.list);
		}
		break;
	case NBT_TAG_Compound:
		INIT_LIST_HEAD(&tag->t_u.t_compound.list);
		do {
			c = hgang_alloc0(nbt->nodes);
			if ( NULL == c )
				return NULL;
			tmp = decode_tag(nbt, ptr, end - ptr, c, TAG_NAMED,
					depth + 1);
			if ( NULL == tmp )
				return NULL;
			ptr = tmp;
			list_add_tail(&c->t_list, &tag->t_u.t_compound.list);
		}while(c->t_type != NBT_TAG_End && ptr < end);
		break;
	default:
		printf("nbt: uknown type %d\n", tag->t_type);
		return NULL;
	}

	return ptr;
}

#if 0
int nbt_strcmp(const struct nbt_string *v1, const struct nbt_string *v2)
{
	size_t min, i;
	int ret;

	min = (v1->len < v2->len) ? v1->len : v2->len;
	ret = v1->len - v2->len;

	for(i = 0; i < min; i++) {
		int ret;

		ret = v1->str[i] - v2->str[i];
		if ( ret )
			break;
	}

	return ret;
}

int nbt_cstrcmp(const struct nbt_string *v1, const char *str)
{
	struct nbt_string v2;

	v2.str = str;
	v2.len = strlen(str);

	return nbt_strcmp(v1, &v2);
}
#endif

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

	ptr = decode_tag(nbt, buf, len, &nbt->root, TAG_NAMED, 1);
	if ( NULL == ptr )
		goto out_free_all;

	goto out;

out_free_all:
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
		hgang_free(nbt->nodes);
		free(nbt);
	}
}
