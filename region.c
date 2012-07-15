/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Load region files. Regions contain chunks. These exist because windows
 * can't handle lots of separate small chunk files so it's really kind of
 * like an archive format for chunks.
*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <endian.h>

#include <zlib.h>

#include <libmc/minecraft.h>
#include <libmc/schematic.h>
#include <libmc/chunk.h>
#include <libmc/region.h>

/* chunk data stored at 4KB granularity */
#define INTERNAL_CHUNK_SHIFT	12
#define INTERNAL_CHUNK_SIZE	(1 << INTERNAL_CHUNK_SHIFT)
#define CSIZE_IN_PAGES(x)	((x + (INTERNAL_CHUNK_SIZE - 1)) >> \
					INTERNAL_CHUNK_SHIFT)

#define RCHUNK_GZIP		1
#define RCHUNK_ZLIB		2

struct rchunk_hdr {
	uint32_t c_len;
	uint8_t c_encoding;
} __attribute__((packed));

struct _region {
	uint32_t locs[REGION_X * REGION_Z];
	uint32_t ts[REGION_X * REGION_Z];
	chunk_t chunks[REGION_X * REGION_Z];
	char *path;
	unsigned int ref;
	int fd;
	int32_t x, z;
	uint8_t dirty;
};

region_t region_open(const char *fn)
{
	struct _region *r;
	ssize_t ret;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		goto out;

	r->path = strdup(fn);
	if ( NULL == r->path )
		goto out;

	r->fd = open(fn, O_RDONLY);
	if ( r->fd < 0 )
		goto out_free;

	ret = pread(r->fd, r->locs, sizeof(r->locs), 0);
	if ( ret < 0 || (size_t)ret < sizeof(r->locs) )
		goto out_close;

	ret = pread(r->fd, r->ts, sizeof(r->ts), INTERNAL_CHUNK_SIZE);
	if ( ret < 0 || (size_t)ret < sizeof(r->ts) )
		goto out_close;
	r->ref = 1;
	goto out;

out_close:
	close(r->fd);
out_free:
	free(r->path);
	free(r);
	r = NULL;
out:
	return r;
}

region_t region_new(const char *fn)
{
	struct _region *r;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		goto out;

	r->path = strdup(fn);
	if ( NULL == r->path )
		goto out_free;

	r->fd = -1;
	r->ref = 1;
	goto out;

out_free:
	free(r);
	r = NULL;
out:
	return r;
}

void region_set_pos(region_t r, int32_t x, int32_t z)
{
	r->x = x;
	r->z = z;
}

static int read_from_loc(struct _region *r, unsigned int i,
			uint8_t **buf, size_t *sz)
{
	uint32_t l, off;
	ssize_t ret;
	size_t len;

	l = be32toh(r->locs[i]);
	off = (l >> 8) << INTERNAL_CHUNK_SHIFT;
	len = (l & 0xff) << INTERNAL_CHUNK_SHIFT;

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

static int chunk_lookup(struct _region *r, uint8_t x, uint8_t z,
			off_t *off, size_t *len)
{
	uint32_t l;

	if ( x >= REGION_X || z >= REGION_Z )
		return 0;

	l = be32toh(r->locs[x * REGION_X + z]);
	if ( off )
		*off = (l >> 8) << INTERNAL_CHUNK_SHIFT;
	if ( len )
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
			printf("er...\n");
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

uint32_t region_get_timestamp(region_t r, uint8_t x, uint8_t z)
{
	if ( x >= REGION_X || z >= REGION_Z )
		return 0;
	return be32toh(r->ts[x * REGION_X + z]);
}

void region_set_timestamp(region_t r, uint8_t x, uint8_t z, uint32_t ts)
{
	if ( x >= REGION_X || z >= REGION_Z )
		return;
	r->ts[x * REGION_X + z] = htobe32(ts);
}

chunk_t region_get_chunk(region_t r, uint8_t x, uint8_t z)
{
	const struct rchunk_hdr *hdr;
	size_t len, dlen;
	uint8_t *buf, *ptr;
	chunk_t c;

	if ( !get_chunk(r, x, z, &buf, &len) )
		return 0;

	/* XXX: not allocated */
	if ( buf == NULL ) {
		return NULL;
	}

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
	free(buf);
	if ( NULL == ptr )
		goto err;

	c = chunk_from_bytes(ptr, dlen);
	free(ptr);

	/* don't increment refcount because we don't
	 * keep a reference to it, this belongs to caller
	 */
	return c;
err_free:
	free(buf);
err:
	return NULL;
}

int region_set_chunk(region_t r, uint8_t x, uint8_t z, chunk_t c)
{
	if ( x >= REGION_X || z >= REGION_Z )
		return 0;
	if ( r->chunks[x * REGION_X + z] )
		chunk_put(r->chunks[x * REGION_X + z]);
	r->dirty = 1;
	r->chunks[x * REGION_X + z] = chunk_get(c);
	return 1;
}

int region_save(region_t r)
{
	struct rchunk_hdr *hdr;
	unsigned int i, pgno;
	ssize_t ret;
	char *path;
	int rc = 0;
	int fd;

	if ( !r->dirty )
		return 1;

	/* write to temporary file */
	if ( asprintf(&path, "%s.tmp", r->path) < 0 )
		goto out;

	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if ( fd < 0 )
		goto out_free;

	/* write out chunk data */
	for(i = 0, pgno = 2; i < REGION_X * REGION_Z; i++) {
		if ( r->chunks[i] ) {
			size_t clen, tlen;
			const uint8_t *cbuf;
			uint8_t *buf, *ptr;
			int32_t x, z;

			x = (r->x * REGION_X) + (i % REGION_X);
			z = (r->z * REGION_Z) + (i / REGION_X);

			if ( !chunk_set_pos(r->chunks[i], x, z) )
				goto out_close;

			/* get compressed chunk data */
			cbuf = chunk_encode(r->chunks[i],
						CHUNK_ENC_ZLIB, &clen);
			if ( NULL == cbuf )
				goto out_close;

			/* wrap it up with header */
			tlen = clen + sizeof(*hdr);
			buf = malloc(tlen);
			if ( NULL == buf ) {
				goto out_close;
			}

			hdr = (struct rchunk_hdr *)buf;
			hdr->c_len = htobe32(clen);
			hdr->c_encoding = RCHUNK_ZLIB;

			ptr = buf + sizeof(*hdr);
			memcpy(ptr, cbuf, clen);

			/* write it out */
			ret = pwrite(fd, buf, tlen,
					pgno << INTERNAL_CHUNK_SHIFT);
			free(buf);
			if ( ret < 0 || (size_t)ret != tlen )
				goto out_close;

			/* last, update location table */
			r->locs[i] = htobe32(pgno << 8 |
					(CSIZE_IN_PAGES(tlen) & 0xff));

			pgno += CSIZE_IN_PAGES(tlen);
			chunk_put(r->chunks[i]);
			r->chunks[i] = NULL;
		}else if ( r->locs[i] ) {
			uint8_t *buf;
			size_t sz;

			/* copy existing */
			if ( !read_from_loc(r, i, &buf, &sz) )
				goto out_close;
			ret = pwrite(fd, buf, sz,
					pgno << INTERNAL_CHUNK_SHIFT);
			free(buf);
			if ( ret < 0 || (size_t)ret != sz )
				goto out_close;
			pgno += CSIZE_IN_PAGES(sz);
		}
	}

	/* up-truncate the file so we don't get EOF when
	 * reading final chunk as multiples of the internal
	 * page-size. We didn't write padding because we want
	 * the file to be sparse if possible.
	*/
	if ( ftruncate(fd, pgno << INTERNAL_CHUNK_SHIFT) )
		goto out_close;

	/* write header */
	ret = pwrite(fd, r->locs, sizeof(r->locs), 0);
	if ( ret < 0 || (size_t)ret < sizeof(r->locs) )
		goto out_close;

	ret = pwrite(fd, r->ts, sizeof(r->ts), INTERNAL_CHUNK_SIZE);
	if ( ret < 0 || (size_t)ret < sizeof(r->ts) )
		goto out_close;

	/* we need to sync + close the file to catch any errors */
//	if ( fsync(fd) && errno != EROFS && errno != EINVAL )
//		goto out_close;
	if ( close(fd) < 0 )
		goto out_free;

	/* rename over actual path */
	if ( rename(path, r->path) )
		goto out_free;

	/* refresh our fd by opening the new file read-only */
	fd = open(r->path, O_RDONLY);
	if ( fd < 0 )
		goto out_free;

	if ( r->fd >= 0 )
		close(r->fd);

	r->fd = fd;
	r->dirty = 0;
	rc = 1;
	goto out_free;

out_close:
	close(fd);
	unlink(path);
out_free:
	free(path);
out:
	return rc;
}

static void region_close(region_t r)
{
	unsigned int i;
	free(r->path);
	if ( r->fd >= 0 )
		close(r->fd);
	for(i = 0; i < REGION_X * REGION_Z; i++ )
		if ( r->chunks[i] )
			chunk_put(r->chunks[i]);
	free(r);
}

void region_put(region_t r)
{
	if ( 0 == --r->ref )
		region_close(r);
}

region_t region_get(region_t r)
{
	r->ref++;
	return r;
}

#define XFLOOR(x) (x / CHUNK_X)
#define ZFLOOR(z) (z / CHUNK_Z)
#define XCEIL(x) ((x + (CHUNK_X-1)) / CHUNK_X)
#define ZCEIL(z) ((z + (CHUNK_Z-1)) / CHUNK_Z)

int region_paste_schematic(region_t r, schematic_t s, int x, int y, int z)
{
	int16_t sx, sz;
	int tx, tz, xmin, zmin, xmax, zmax;

	schematic_get_size(s, &sx, NULL, &sz);

	tx = x + sx;
	tz = z + sz;

	xmin = XFLOOR(s_min(x, tx));
	zmin = ZFLOOR(s_min(z, tz));
	xmax = XCEIL(s_max(x, tx));
	zmax = ZCEIL(s_max(z, tz));

	printf("region: schematic dimensions %d %d: %d,%d -> %d,%d\n",
		sx, sz, xmin, zmin, xmax, zmax);

	assert(xmin >= 0 && zmin >= 0);
	assert(xmax < (CHUNK_X * REGION_X));
	assert(zmax < (CHUNK_Z * REGION_Z));

	x %= CHUNK_X;
	z %= CHUNK_Z;

	for(tx = xmin; tx < xmax; tx++, x -= CHUNK_X) {
		for(tz = zmin; tz < zmax; tz++, z -= CHUNK_Z) {
			chunk_t c;
			int ret;

			if ( chunk_lookup(r, tx, tz, NULL, NULL) ) {
				c = region_get_chunk(r, tx, tz);
			}else{
				c = chunk_new();
			}
			if ( NULL == c )
				return 0;
			if ( !region_set_chunk(r, tx, tz, c) ) {
				chunk_put(c);
				return 0;
			}
			/* TODO: get sub-schematic */
			ret = chunk_paste_schematic(c, s, x, y, z);
			if ( ret )
				ret = chunk_set_terrain_populated(c, 1);
			chunk_put(c);
			if ( !ret )
				return 0;
		}
	}

	return 1;
}
