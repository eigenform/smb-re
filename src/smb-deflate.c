#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "smb.h"
#include "smb_compression.h"

int load_region_to_buffer(char *filename, char *target_buf){
	FILE *f = fopen(filename, "rb");
	long fsize;
	int sz;

	if (f == NULL) {
		printf("[!] Couldn't open %s\n", filename);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (fsize != MAX_SIZE)
		return -1;

	if (fread(target_buf, 1, MAX_SIZE, f) < MAX_SIZE)
		return -1;

	fclose(f);
	return 0;
}


int main(int argc, char* argv[]){
	if (argc < 3) {
		printf("usage: smb-deflate <input region> <output file>\n");
		return -1; 
	}

	unsigned char region[MAX_SIZE*2];
	if (load_region_to_buffer(argv[1], region) < 0) {
		printf("[!] Failed to load inflated region file, quitting\n");
		return -1;
	}

	FILE *f;
	f = fopen(argv[2], "wb");
	if (f == NULL) {
		printf("[!] Couldn't open output file, qutting\n");
		return -1;
	}


	printf("[*] Inflated region: %s\n", argv[1]);
	printf("[*]          Output: %s\n", argv[2]);


	unsigned char out[MAX_SIZE];
	int deflated_size = deflate_region(region, out);

	fwrite(&out, 1, deflated_size, f);
	fclose(f);
	printf("[*] Wrote deflated region to %s\n", argv[2]);
	return 0;
}
