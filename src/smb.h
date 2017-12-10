#ifndef _SMB_H
#define _SMB_H

#include <stdint.h>
#include <byteswap.h>

// The maximum size of inflated replay data
#define MAX_SIZE 0x16844

// Offset to actual savedata in GCI
#define GCI_METADATA_SIZE 0x2040

// Offset to start of deflated replay data, from start of savedata
#define REPLAY_HEADER_SIZE 0x58

#define BLOCK_SIZE 0x2000
#define COMMENT_SIZE 0x40
#define DENTRY_SIZE 0x40
#define BANNER_SIZE 0x2000

struct dentry {
	uint8_t gamecode[4];
	uint8_t makercode[2];
	uint8_t unused_a;
	uint8_t bi_flags;
	uint8_t filename[0x20];
	uint8_t modtime[4];
	uint8_t image_offset[4];
	uint8_t icon_fmt[2];
	uint8_t anim_speed[2];
	uint8_t permissions;
	uint8_t copy_counter;
	uint8_t first_block[2];
	uint8_t block_count[2];
	uint8_t unused_b[2];
	uint8_t comments_addr[4];
}__attribute__((__packed__));

struct smb_data {
	unsigned char checksum[2];
	unsigned char unk_1;
	unsigned char menu_lvl_info[2];
	unsigned char menu_lvl_diff;
	unsigned char menu_level;
	unsigned char unk_2;
	unsigned char menu_score[4];
	unsigned char timestamp[4];
	unsigned char banner[BANNER_SIZE];
	unsigned char comments[COMMENT_SIZE];
	unsigned char replay_size[8];
	unsigned char replay[0];
}__attribute__((__packed__));

struct smb_gamedata {
	unsigned char checksum[2];
	unsigned char unk_5[2];
	unsigned char banner[0x5800];
	unsigned char comments[COMMENT_SIZE];
	unsigned char unk_6[4];
	unsigned char savedata[0];
}__attribute__((__packed__));

struct gci {
	struct dentry dentry;
	struct smb_data data;
}__attribute__((__packed__));

struct gci_gamedata {
	struct dentry dentry;
	struct smb_gamedata data;
}__attribute__((__packed__));

// This isn't portable
#define b16(x)	bswap_16(x)
#define b32(x)	bswap_32(x)
#define b64(x)	bswap_64(x)

#endif // _SMB_H
