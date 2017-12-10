#include <stdio.h>
#include <string.h>

#include "smb_compression.h"
#include "smb.h"

/* 
 * Inflate binary replay data from some .gci, obtaining the representation 
 * resident in memory (starting at ~0x80250b80) during playback.  
 */

int main(int argc, char* argv[]){
	if (argc < 3) {
		printf("usage: smb-inflate <input GCI> <output file>\n");
		return -1; 
	}

	FILE *f, *r;
	unsigned char buf[MAX_SIZE*2];
	unsigned char res[MAX_SIZE*2];
	printf("[*] GCI   : %s\n", argv[1]);
	printf("[*] Output: %s\n", argv[2]);

	f = fopen(argv[1], "rb");
	if (f == NULL) {
		printf("[!] Couldn't open %s\n", argv[1]);
		return -1;
	}
	fread(&buf, 1, MAX_SIZE, f);
	fclose(f);

	r = fopen(argv[2], "wb");
	if (r == NULL) {
		printf("[!] Couldn't open %s for writing\n", argv[2]);
		return -1;
	}

	int output_size = inflate_region(buf, res);

	fwrite(&res, 1, output_size, r);
	fclose(r);
	printf("[*] Wrote inflated region to %s\n", argv[2]);
	return 0;
}
