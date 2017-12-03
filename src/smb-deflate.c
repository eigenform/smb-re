#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "smb.h"

int load_region(char *filename, char *target_buf){
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

/* Yes, this is absolutely disgusting and highly convoluted, but it works. 
 * Sorry if you have to read this. Painfully "brute-forced" this problem and
 * came out with only a vague understanding of the underlying algorithm.
 *
 * Seems like a kind of run-length encoding where:
 *
 *	- Runs of repeated bytes up to 127 bytes are encoded as the pair:
 *		(0x80 | runcount), <byte to repeat>
 *	- A run of 128 repeated bytes is encoded as the pair:
 *		0xFF, <byte to repeat>
 *	- Strings of distinct bytes up to 127 bytes are encoded as:
 *		<size N>, <byte 1>, <byte 2>, ... <byte N>
 *
 * There appears to be some weird behaviour where some string of literal bytes
 * will count and include the first byte in some adjacent run of byte 0x00.
 * It's unclear [to me, at least right now,] why this is the case.
 *		
 * I think the byte that encodes the size of some run or string of literals 
 * uses the high bit to distinguish between the two.
 * 
 *	For runs of repeated bytes, the high bit is always set:
 *
 *		1 0 0 0 0 0 0 0 
 *		  ^ ^ ^ ^ ^ ^ ^
 *		    run size
 *
 *	For runs of literal bytes, the high bit is unset:
 *
 *		0 0 0 0 0 0 0 0
 *		  ^ ^ ^ ^ ^ ^ ^	
 *		    lit. size
 */
int deflate_region(unsigned char region[], unsigned char out[]){

	unsigned char buf[0x1000];
	unsigned char lookahead_val;
	unsigned char cursor_val;

	int write_cursor = 0;
	int lookback_cursor = 0;
	int bufcursor = 0;
	int cursor = 0;

	int copycount = 1;
	int runcount = 1;

	for (cursor; cursor <= MAX_SIZE; cursor++) {
		cursor_val	= region[cursor];
		lookahead_val	= region[cursor+1];

		if(cursor_val == lookahead_val) {
			runcount +=1;

			if (runcount > 1 && cursor_val == 0 && copycount > 1)
				buf[bufcursor++] = cursor_val;

			if (copycount > 1) {
				if (copycount == 0x7f) {
					buf[bufcursor++] = cursor_val;
					out[write_cursor++] = copycount;
					for (int i=0; i < copycount; i++)
						out[write_cursor++] = buf[i];

					runcount = 1;
					bufcursor = 0;
					copycount = 1;
					continue;

				}

				buf[bufcursor++] = cursor_val;
				out[write_cursor++] = copycount;
				for (int i=0; i < copycount; i++)
					out[write_cursor++] = buf[i];

				runcount = 1;
				bufcursor = 0;
				copycount = 1;
				continue;
			}
			
			if (runcount == 0x80) {
				out[write_cursor++] = 0xFF;
				out[write_cursor++] = cursor_val;
				runcount = 1;
				continue;
			}

			continue;
		}

		if(cursor_val != lookahead_val) {
			if (runcount > 1) {
				out[write_cursor++] = (0x80 | runcount);
				out[write_cursor++] = cursor_val;
				runcount = 1;
				continue;
			}

			if (copycount == 0x7f) {
				buf[bufcursor++] = cursor_val;
				out[write_cursor++] = copycount;
				for (int i=0; i < copycount; i++)
					out[write_cursor++] = buf[i];

				runcount = 1;
				bufcursor = 0;
				copycount = 1;
				continue;
			}

			buf[bufcursor++] = cursor_val;
			copycount += 1;
		}
	}

	return write_cursor;
}


int main(int argc, char* argv[]){
	if (argc < 3) {
		printf("usage: smb-deflate <input region> <output file>\n");
		return -1; 
	}

	unsigned char region[MAX_SIZE*2];
	if (load_region(argv[1], region) < 0) {
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
