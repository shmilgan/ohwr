#include <stdio.h>
#include "libtools.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Use: \"%s <filename>\"\n", argv[0]);
	}
	return load_fpga_main(argv[1]);
}
