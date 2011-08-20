/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
*/
#include <stdint.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libmc/nbt.h>

static int mapfile(int fd, const uint8_t **begin, size_t *sz)
{
	struct stat st;

	if ( fstat(fd, &st) ) {
		perror("fstat()");
		return 0;
	}

	*begin = mmap(NULL, st.st_size, PROT_READ,
			MAP_SHARED, fd, 0);

	if ( *begin == MAP_FAILED ) {
		return 0;
	}

	*sz = st.st_size;

	return 1;
}

int main(int argc, char **argv)
{
	const uint8_t *map;
	size_t sz;
	nbt_t nbt;

	if ( !mapfile(STDIN_FILENO, &map, &sz) )
		return EXIT_FAILURE;

	nbt = nbt_decode(map, sz);
	if ( NULL == nbt )
		return EXIT_FAILURE;

	munmap((void *)map, sz);

	nbt_dump(nbt);
	nbt_free(nbt);
	return EXIT_SUCCESS;
}
