#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fuse.h>

#include "types.h"
#include "helper.h"
#include "wearlevel.h"
#include "crypto.h"
#include "fs.h"
#include "fuse_glue.h"


int main(int argc, char *argv[]) {
	FILE *f;
	u8 *save_buf, *out_buf, keybuf[0x200];
	unsigned int size=0;
	int i;

	int fargc;
	char **fargv;

	f = fopen(argv[1], "rb");

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	save_buf = malloc(size);
	out_buf = malloc(size);
	fread(save_buf, size, 1, f);

	fclose(f);

	if (find_key(save_buf, size, keybuf) == -1) {
		fprintf(stderr, "error: could not find key block :(\n");
		return -1;
	}

	rearrange(save_buf, out_buf, size);
	xor(out_buf, size - 0x1000, NULL, keybuf, 0x200);

	fargc = argc - 1;
	fargv = (char **) malloc(fargc * sizeof(char *));	

	fargv[0] = argv[0];

	for(i = 1; i < fargc; i++) {
#ifdef DEBUG
		printf("arg: '%s'\n", argv[i + 1]);
#endif
		fargv[i] = argv[i + 1];
	}

#ifdef DEBUG
	printf("** FUSE GO! **\n");
#endif

//	printf("num_part: %d\n", fs_num_partition(out_buf));

	return fuse_sav_init(out_buf, size - 0x1000, keybuf, fargc, fargv);

	return 0;
}
