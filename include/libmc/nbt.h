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
#define NBT_TAG_Int_Array	11U
#define NBT_TAG_MAX		12U

typedef struct _nbt *nbt_t;
typedef struct nbt_tag *nbt_tag_t;

nbt_t nbt_decode(const uint8_t *buf, size_t len);
nbt_t nbt_new(void);
size_t nbt_size_in_bytes(nbt_t nbt);
int nbt_get_bytes(nbt_t nbt, uint8_t *buf, size_t len);
void nbt_free(nbt_t nbt);

nbt_tag_t nbt_root_tag(nbt_t nbt);

nbt_tag_t nbt_tag_new(nbt_t nbt, uint8_t type);
nbt_tag_t nbt_tag_new_list(nbt_t nbt, uint8_t type);

uint8_t nbt_tag_type(nbt_tag_t t);
char *nbt_tag_name(nbt_tag_t t);

/* Get values from tags */
int nbt_byte_get(nbt_tag_t t, uint8_t *val);
int nbt_short_get(nbt_tag_t t, int16_t *val);
int nbt_int_get(nbt_tag_t t, int32_t *val);
int nbt_long_get(nbt_tag_t t, int64_t *val);
int nbt_bytearray_get(nbt_tag_t t, uint8_t **bytes, size_t *sz);
int nbt_intarray_get(nbt_tag_t t, int32_t **bytes, unsigned int *num);
int nbt_string_get(nbt_tag_t t, char **val);
nbt_tag_t nbt_list_get(nbt_tag_t t, unsigned idx);
int nbt_list_get_size(nbt_tag_t t);
nbt_tag_t nbt_compound_get(nbt_tag_t t, const char *key);

/* Set values in to tags */
int nbt_byte_set(nbt_tag_t t, uint8_t val);
int nbt_short_set(nbt_tag_t t, int16_t val);
int nbt_int_set(nbt_tag_t t, int32_t val);
int nbt_long_set(nbt_tag_t t, int64_t val);
int nbt_bytearray_set(nbt_tag_t t, const uint8_t *bytes, unsigned int num);
int nbt_intarray_set(nbt_tag_t t, const int32_t *arr, unsigned int num);
int nbt_string_set(nbt_tag_t t, const char *val);
int nbt_list_set(nbt_tag_t t, unsigned idx, nbt_tag_t val);
int nbt_list_set_size(nbt_tag_t t, unsigned sz);
int nbt_list_append(nbt_tag_t t, nbt_tag_t val);
int nbt_compound_delete(nbt_tag_t t, const char *key);
int nbt_compound_set(nbt_tag_t t, const char *key, nbt_tag_t val);

/* delete all items in lists/compounds */
int nbt_list_nuke(nbt_tag_t t);
int nbt_compound_nuke(nbt_tag_t t);

void nbt_dump(nbt_t nbt);

#endif /* _NBT_H */
