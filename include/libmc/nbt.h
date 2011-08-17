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

struct nbt_string {
	int len;
	const char *str;
};

struct nbt_byte_array {
	int32_t len;
	uint8_t *array;
};

struct nbt_list {
	uint8_t type;
	int32_t len;
	const uint8_t *ptr;
};

struct nbt_compound {
	const uint8_t *ptr;
	size_t max_len;
};

struct nbt_tag {
	struct nbt_string t_name;
	const uint8_t *t_ptr;
	size_t t_len;
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
};

const uint8_t *nbt_decode(const uint8_t *ptr, size_t len, struct nbt_tag *tag);
int nbt_strcmp(const struct nbt_string *v1, const struct nbt_string *v2);
int nbt_cstrcmp(const struct nbt_string *v1, const char *str);

#endif /* _NBT_H */
