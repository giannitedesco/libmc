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

nbt_t nbt_decode(const uint8_t *buf, size_t len);
void nbt_free(nbt_t nbt);

#endif /* _NBT_H */
