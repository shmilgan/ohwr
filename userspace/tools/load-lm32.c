#include <stdio.h>
#include "libtools.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Use: \"%s <filename>\"\n", argv[0]);
	}
	return load_lm32_main(argv[1]);
}
