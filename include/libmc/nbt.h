#ifndef _NBT_H
#define _NBT_H

#define NBT_TAG_End		0U
#define NBT_TAG_Byte		1U
#define NBT_TAG_Short		2U
#define NBT_TAG_Int		3U
#define NBT_TAG_Long		4U
#define NBT_TAG_Float		5U
#define NBT_TAG_Double		6U
#define NBT_TAG_Byte_Array	7U
#define NBT_TAG_String		8U
#define NBT_TAG_List		9U
#define NBT_TAG_Compound	10U

typedef struct _nbt *nbt_t;
typedef struct nbt_tag *nbt_tag_t;

nbt_t nbt_decode(const uint8_t *buf, size_t len);
nbt_tag_t nbt_root_tag(nbt_t nbt);
void nbt_free(nbt_t nbt);

uint8_t nbt_tag_type(nbt_tag_t t);
char *nbt_tag_name(nbt_tag_t t);

int nbt_byte_get(nbt_tag_t t, uint8_t *val);
int nbt_short_get(nbt_tag_t t, int16_t *val);
int nbt_int_get(nbt_tag_t t, int32_t *val);
int nbt_long_get(nbt_tag_t t, int64_t *val);
int nbt_buffer_get(nbt_tag_t t, const uint8_t **bytes, size_t *sz);
int nbt_string_get(nbt_tag_t t, char **val);
int nbt_list_get(nbt_tag_t t, unsigned idx, nbt_tag_t *val);
int nbt_list_size(nbt_tag_t t);

nbt_tag_t nbt_compound_get_child(nbt_tag_t t, const char *name);

void nbt_dump(nbt_t nbt);

#endif /* _NBT_H */
