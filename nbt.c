#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmc/nbt.h>

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

const uint8_t *nbt_decode(const uint8_t *ptr, size_t len, struct nbt_tag *tag)
{
	const uint8_t *end = ptr + len;

	if ( len < 1 )
		return NULL;

	tag->t_ptr = ptr;
	tag->t_len = len;
	tag->t_type = *ptr;

	ptr++;

	if ( tag->t_type != NBT_TAG_End ) {
		if ( ptr + 2 > end )
			return NULL;
		tag->t_name.len = be16toh(*(int16_t *)ptr);
		tag->t_name.str = (char *)(ptr + 2);
		ptr += 2 + tag->t_name.len;
		if ( tag->t_name.len < 0 || ptr > end )
			return 0;
	}else{
		tag->t_name.len = 0;
		tag->t_name.str = NULL;
	}

	switch(tag->t_type) {
	case NBT_TAG_End:
		break;
	case NBT_TAG_Byte:
	case NBT_TAG_Short:
	case NBT_TAG_Int:
	case NBT_TAG_Long:
	case NBT_TAG_Float:
	case NBT_TAG_Double:
		return NULL;
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
	case NBT_TAG_List:
		return NULL;
	case NBT_TAG_Compound:
		tag->t_u.t_compound.ptr = ptr;
		tag->t_u.t_compound.max_len = end - ptr;
		break;
	default:
		printf("nbt: uknown type %d\n", tag->t_type);
		return NULL;
	}

	return ptr;
}

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
