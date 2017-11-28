#include <stdio.h>
#include <string.h>

/* 
 * Inflate binary replay data from some .gci, obtaining the representation 
 * resident in memory (starting at ~0x80250b80) during playback.  
 */

#define MAX_SIZE 0x16840

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

	unsigned char cur, targ;
	int count;

	// Compressed data starts at +0x2098 in .gci
	int ipos = 0x2098, opos = 0x0;
	while (ipos < MAX_SIZE && opos < MAX_SIZE)
	{
		cur = buf[ipos];
		if (cur & (1<<7)) {
			count = (int)(cur & ~(1 <<7));
			targ = buf[ipos+1];
			memset(&res[opos], targ, count);
			ipos += 2;
			opos += count;
		} else {
			count = (int)cur;
			ipos++;
			memcpy(&res[opos], &buf[ipos], count);
			ipos += count;
			opos += count;
		}
	}
	fwrite(&res, 1, MAX_SIZE, r);
	fclose(r);
	return 0;
}
