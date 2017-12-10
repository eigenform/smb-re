#include <stdio.h>
#include <string.h>
#include "smb_compression.h"
#include "smb.h"

int inflate_region(unsigned char *buf, unsigned char *res){
	unsigned char cursor_val, targ;
	int count;

	// Compressed data starts at +0x2098 in .gci
	//int cursor = 0x2098;
	int cursor = 0x00;
	int write_cursor = 0x0;
	while (cursor < MAX_SIZE && write_cursor < MAX_SIZE) {
		cursor_val = buf[cursor];
		if (cursor_val & (1<<7)) {
			count = (int)(cursor_val & ~(1 <<7));
			targ = buf[cursor+1];
			memset(&res[write_cursor], targ, count);
			cursor += 2;
			write_cursor += count;
		} 
		else {
			count = (int)cursor_val;
			cursor++;
			memcpy(&res[write_cursor], &buf[cursor], count);
			cursor += count;
			write_cursor += count;
		}
	}

	return write_cursor;
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
//int deflate_region(unsigned char region[], unsigned char out[]){
int deflate_region(unsigned char* region, unsigned char* out){

	unsigned char buf[0x1000];
	unsigned char lookahead_val;
	unsigned char cursor_val;

	int write_cursor = 0;
	int lookback_cursor = 0;
	int bufcursor = 0;
	int cursor = 0;

	int copycount = 1;
	int runcount = 1;

	for (cursor; cursor < MAX_SIZE; cursor++) {
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

	out[write_cursor++] = (0x80 | runcount - 1);
	out[write_cursor++] = cursor_val;
	return write_cursor;
}


/* Fix GCI checksum; only slightly adapted from original code, see:
 * 	https://github.com/bobjrsenior/SMB_Checksum_Fixer
 */
void fix_checksum(unsigned char *output_gci, uint32_t gci_size, uint8_t type) {
	uint32_t lim;
	uint32_t checksum_offset = 0x42;
	uint32_t i = checksum_offset;

	switch(type){

	/* Because normal gamedata files are always 3 blocks, we can
	 * assume that the checksum always runs from offset 0x42 to
	 * the end of actual data at 0x5c02
	 */
	case GAME_DATA:
		lim = checksum_offset + 0x5c02;
		break;
	
	/* Checksums on replay data always read to the end of the 
	 * whole file
	 */
	case REPLAY_DATA:
		lim = gci_size;
		break;
	}

	uint16_t generatorPolynomial = 0x1021;
	uint16_t checksum = 0xFFFF;

	for (i; i < lim; i++) {
		checksum ^= ((uint8_t)output_gci[i] << 8);
		for (int j = 0; j < 8; j++) {
			if (checksum & 0x8000) {
				checksum <<= 1;
				checksum ^= generatorPolynomial;
			}
			else checksum <<= 1;

			checksum &= 0xFFFF;
		}
	}

	// Flip bits in checksum
	checksum ^= 0xFFFF;

	uint16_t existing_checksum = (output_gci[0x40] << 8) | output_gci[0x41];
	if (existing_checksum == checksum) {
		printf("[*] Checksum on GCI is already valid\n");
		return;
	}
	output_gci[0x40] = checksum >> 8;
	output_gci[0x41] = checksum;
	printf("[*] Fixed checksum (0x%04x -> 0x%04x)\n",
			existing_checksum, checksum);
}


