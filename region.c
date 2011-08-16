#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <endian.h>

#include <libmc/chunk.h>
#include <libmc/region.h>

/* chunk data stored at 4KB granularity */
#define INTERNAL_CHUNK_SHIFT 12

struct rchunk_hdr {
	uint32_t c_len;
	uint32_t c_encoding;
} __attribute__((packed));

struct _region {
	int fd;
	uint32_t locs[REGION_X * REGION_Z];
};

region_t region_open(const char *fn)
{
	struct _region *r;
	ssize_t ret;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		goto out;
	
	r->fd = open(fn, O_RDONLY);
	if ( r->fd < 0 )
		goto out_free;

	ret = pread(r->fd, r->locs, sizeof(r->locs), 0);
	if ( ret < 0 || (size_t)ret < sizeof(r->locs) )
		goto out_close;

	goto out;

out_close:
	close(r->fd);
out_free:
	free(r);
	r = NULL;
out:
	return r;
}

static int chunk_lookup(struct _region *r, uint8_t x, uint8_t z,
			off_t *off, size_t *len)
{
	uint32_t l;
	if ( x > REGION_X || z > REGION_Z )
		return 0;

	l = be32toh(r->locs[x * REGION_X + z]);
	*off = (l >> 8) << INTERNAL_CHUNK_SHIFT;
	*len = (l & 0xff) << INTERNAL_CHUNK_SHIFT;

	if ( *off && *len ) {
		printf("(%d,%d) %zu bytes at %"PRId64"\n",
			x, z, *len, *off);
	}
	
	return 1;
}

chunk_t region_get_chunk(region_t r, uint8_t x, uint8_t z)
{
	const struct rchunk_hdr *hdr;
	off_t off;
	size_t len;
	uint8_t *buf, *ptr;
	ssize_t ret;
	chunk_t c;

	if ( !chunk_lookup(r, x, z, &off, &len) )
		return NULL;

	/* chunk not populated */
	if ( !off || !len )
		return NULL;

	buf = malloc(len);
	if ( NULL == buf )
		return NULL;

	ret = pread(r->fd, buf, len, off);
	if ( ret < 0 || (size_t)ret < len )
		goto err_free;

	hdr = (struct rchunk_hdr *)buf;
	len -= sizeof(*hdr);

	if ( be32toh(hdr->c_len) > len )
		goto err_free;
	len = be32toh(hdr->c_len);

	switch(be32toh(hdr->c_encoding)) {
	case 1:
		if ( len < 6 )
			goto err_free;
		ptr = buf + 2;
		len -= 6;
		break;
	case 2:
		ptr = buf;
		break;
	default:
		printf("Uknown chunk encoding\n");
		goto err_free;
	}

	// zlib decompress (ptr, len) in to new buffer
	//c = chunk_from_bytes(buf, len);

	free(buf);

	return c;
err_free:
	free(buf);
	return NULL;
}

void region_close(region_t r)
{
	if ( r ) {
		close(r->fd);
		free(r);
	}
}
