/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load region files. Regions contain chunks. These exist because windows
 * can't handle lots of separate small chunk files so it's really kind of
 * like an archive format for chunks.
*/
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

#include <zlib.h>

#include <libmc/chunk.h>
#include <libmc/region.h>

/* chunk data stored at 4KB granularity */
#define INTERNAL_CHUNK_SHIFT 	12

#define RCHUNK_GZIP		1
#define RCHUNK_ZLIB		2

struct rchunk_hdr {
	uint32_t c_len;
	uint8_t c_encoding;
} __attribute__((packed));

struct _region {
	int fd;
	uint32_t locs[REGION_X * REGION_Z];
	chunk_t chunks[REGION_X * REGION_Z];
	uint8_t dirty;
};

region_t region_open(const char *fn, int rdonly)
{
	struct _region *r;
	ssize_t ret;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		goto out;

	r->fd = open(fn, (rdonly) ? O_RDONLY : O_RDWR);
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

region_t region_new(const char *fn)
{
	struct _region *r;
	ssize_t ret;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		goto out;
	
	r->fd = open(fn, O_RDWR|O_CREAT|O_TRUNC, 0600);
	if ( r->fd < 0 )
		goto out_free;

	/* write empty header */
	ret = pwrite(r->fd, r->locs, sizeof(r->locs), 0);
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
	if ( x >= REGION_X || z >= REGION_Z )
		return 0;

	l = be32toh(r->locs[x * REGION_X + z]);
	*off = (l >> 8) << INTERNAL_CHUNK_SHIFT;
	*len = (l & 0xff) << INTERNAL_CHUNK_SHIFT;

#if 0
	if ( *off && *len ) {
		printf("(%d,%d) %zu bytes at %"PRId64"\n",
			x, z, *len, *off);
	}
#endif

	return 1;
}

static uint8_t *region_decompress(const uint8_t *buf, size_t len, size_t *dlen)
{
	uint8_t *d = NULL, *new;
	int ret;

	/* Nice compression ratio, shame zlib is so fucking slow and
	 * we don't know the size up-front. RLE would be far better --
	 * the reason the ratio's are so good is lots of contiguous
	 * air (top half will all be air) end plenty contig runs of
	 * stone, dirt or sand.
	 */
	for(*dlen = 88 << 10; ; *dlen += 16 << 10) {
		new = realloc(d, *dlen);
		if ( NULL == new ) {
			free(d);
			return NULL;
		}

		d = new;

		ret = uncompress(d, dlen, buf, len);
		switch(ret) {
		case Z_OK:
			return d;
		case Z_BUF_ERROR:
			*dlen *= 2;
			continue;
		default:
			free(d);
			return NULL;
		}
	}
}

static int get_chunk(struct _region *r, uint8_t x, uint8_t z,
			uint8_t **buf, size_t *sz)
{
	off_t off;
	size_t len;
	ssize_t ret;

	if ( !chunk_lookup(r, x, z, &off, &len) )
		return 0;

	/* chunk not populated */
	if ( !off || !len ) {
		*buf = NULL;
		*sz = 0;
		return 1;
	}

	*buf = malloc(len);
	if ( NULL == *buf )
		return 0;

	ret = pread(r->fd, *buf, len, off);
	if ( ret < 0 || (size_t)ret < len ) {
		free(*buf);
		return 0;
	}

	*sz = len;
	return 1;
}

chunk_t region_get_chunk(region_t r, uint8_t x, uint8_t z)
{
	const struct rchunk_hdr *hdr;
	size_t len, dlen;
	uint8_t *buf, *ptr;
	chunk_t c;

	if ( !get_chunk(r, x, z, &buf, &len) )
		return 0;

	hdr = (struct rchunk_hdr *)buf;
	len -= sizeof(*hdr);
	ptr = buf + sizeof(*hdr);

	if ( be32toh(hdr->c_len) > len )
		goto err_free;
	len = be32toh(hdr->c_len);

	switch(hdr->c_encoding) {
	case RCHUNK_GZIP:
		printf("gzip\n");
		if ( len < 6 )
			goto err_free;
		ptr += 2;
		len -= 6;
		break;
	case RCHUNK_ZLIB:
		break;
	default:
		printf("Uknown chunk encoding\n");
		goto err_free;
	}

	ptr = region_decompress(ptr, len, &dlen);
	if ( NULL == ptr )
		goto err_free;

	/* chunk now owns ptr */
	c = chunk_from_bytes(ptr, dlen);
	free(ptr);
	free(buf);
	return c;
err_free:
	free(buf);
	return NULL;
}

int region_set_chunk(region_t r, uint8_t x, uint8_t z, chunk_t c)
{
	if ( x >= REGION_X || z >= REGION_Z )
		return 0;
	r->dirty = 1;
	r->chunks[x * REGION_X + z] = c;
	return 1;
}

int region_save(region_t r)
{
	int rc = 0;
	unsigned int i;

	if ( !r->dirty )
		return 1;

	for(i = 0; i < REGION_X * REGION_Z; i++) {
		if ( r->chunks[i] ) {
			size_t clen;

			clen = chunk_size_in_bytes(r->chunks[i]);
			/* encode chunk */
		}else if ( r->locs[i] ) {
			/* copy existing */
		}
	}

	r->dirty = 0;
	rc = 1;
	return rc;
}

void region_close(region_t r)
{
	if ( r ) {
		close(r->fd);
		free(r);
	}
}
