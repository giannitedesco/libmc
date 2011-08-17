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

	if ( len < 3 )
		return NULL;

	tag->t_ptr = ptr;
	tag->t_len = len;
	tag->t_type = *ptr;

	ptr++;

	if ( tag->t_type != NBT_TAG_End ) {
		tag->t_name.len = *(int16_t *)ptr;
		tag->t_name.str = ptr + 2;
		ptr += 2 + tag->t_name.len;
		if ( tag->t_name.len < 0 || ptr > end )
			return 0;
	}else{
		tag->t_name.len = 0;
		tag->t_name.str = NULL;
	}

	switch(tag->t_type) {
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
