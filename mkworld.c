/* Copyright (c) Gianni Tedesco 2011
 * Author: Gianni Tedesco (gianni at scaramanga dot co dot uk)
*/
#include <stdint.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <libmc/chunk.h>
#include <libmc/region.h>

int main(int argc, char **argv)
{
	region_t r;

	r = region_new("test.mcr");
	if ( NULL == r )
		return EXIT_FAILURE;

	if ( !region_save(r) )
		return EXIT_FAILURE;

	region_close(r);
	return EXIT_SUCCESS;
}
