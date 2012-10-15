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
	u8 *save_buf, *out_buf, xorpad_buf[0x200];
	u8 zerobuf[0x10];
	unsigned int size=0;
	int i;
	int enable_wearlevel = 1, enable_xorpad = 1;

	int fargc = 1, argi = 1;
	char **fargv;

	f = fopen(argv[1], "rb");
	if(f == NULL) {
		fprintf(stderr, "error: failed to open %s\n", argv[1]);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	save_buf = malloc(size);
	out_buf = malloc(size);
	fread(save_buf, size, 1, f);

	fclose(f);

	for(i = 1; i < argc - 1; i++) {
		if(strncmp(argv[i + 1], "--", 2))fargc++;
	}

	fargv = (char **) malloc(fargc * sizeof(char *));

	fargv[0] = argv[0];

	for(i = 1; i < argc - 1; i++) {
#ifdef DEBUG
		printf("arg: '%s'\n", argv[i + 1]);
#endif
		if(strncmp(argv[i + 1], "--nowear", 8)==0) {
			enable_wearlevel = 0;
		}
		else {
			fargv[argi] = argv[i + 1];
			argi++;
		}
	}

	if(enable_wearlevel) {
		if(rearrange(save_buf, out_buf, size) != 0) {
			free(save_buf);
			free(out_buf);
			return -2;
		}
		else {
			size -= 0x2000;
		}
	}
	else {
		memcpy(out_buf, save_buf, size);
	}

	memset(zerobuf, 0, 0x10);
	if(memcmp(&out_buf[0x10], zerobuf, 0x10)==0)enable_xorpad = 0;

	if(enable_xorpad) {
		if (find_key(out_buf, size, xorpad_buf) == -1) {
			fprintf(stderr, "error: could not find xorpad block :(\n");
			free(save_buf);
			free(out_buf);
			return -1;
		}

		xor(out_buf, size, NULL, xorpad_buf, 0x200);
		
	FILE* f = fopen("logical.bin", "wb");
	fwrite(out_buf, 1, size, f);
	fclose(f);
		
	}

#ifdef DEBUG
	printf("** FUSE GO! **\n");
#endif

	return fuse_sav_init(out_buf, size, xorpad_buf, fargc, fargv);
}
