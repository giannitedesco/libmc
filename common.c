/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
 *
 * Handle level.dat files
*/
#include <libmc/minecraft.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>

int libmc_gunzip(const char *path, uint8_t **begin, size_t *osz)
{
	int fd, ret, rc = 0;
	uint8_t *buf;
	size_t dlen;
	uint32_t d;
	gzFile gz;

	/* gcc, huh?! */
	*begin = NULL;
	*osz = 0;

	fd = open(path, O_RDONLY);
	if ( fd < 0 ) {
		fprintf(stderr, "level: %s: %s\n", path, strerror(errno));
		goto out;
	}

	/* grab decompressed len from gzip trailer, eugh */
	if ( lseek(fd, -sizeof(d), SEEK_END) < 0 )
		goto out_close;
	if ( read(fd, &d, sizeof(d)) != sizeof(d) )
		goto out_close;
	if ( lseek(fd, 0, SEEK_SET) < 0 )
		goto out_close;

	dlen = d;

	buf = malloc(dlen);
	if ( NULL == buf )
		goto out_close;

	gz = gzdopen(fd, "r");
	ret = gzread(gz, buf, dlen);
	gzclose(gz);
	if ( ret < 0 ) {
		free(buf);
		goto out;
	}

	*begin = buf;
	*osz = dlen;
	rc = 1;
	goto out;

out_close:
	close(fd);
out:
	return rc;
}
