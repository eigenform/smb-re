#ifndef _SMB_COMPRESSION
#define _SMB_COMPRESSION

#include <stdint.h>

#define REPLAY_DATA 1
#define GAME_DATA   2

void fix_checksum(unsigned char *output_gci, uint32_t gci_size, uint8_t type);
int inflate_region(unsigned char buf[], unsigned char res[]);
int deflate_region(unsigned char region[], unsigned char out[]);
#endif // _SMB_COMPRESSION
