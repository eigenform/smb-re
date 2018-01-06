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

/* This is the layout for replay savefiles;
 * probably pull in Piston's replaydata format later */
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

/* This is the high-level layout for gamedata savefiles.
 *
 * Zero-length array at the end should be replaced after
 * we've finished articulating the actual format */
struct smb_gamedata {
	unsigned char checksum[2];
	//unsigned char unk_5[2];
	unsigned char gamedata_version[2]; // I think?
	unsigned char banner[0x5800];
	unsigned char comments[COMMENT_SIZE];
	unsigned char unk_6[4];
	unsigned char savedata[0]; //eventually turn into struct
}__attribute__((__packed__));


/* Race party-game tables ---------------------- */
struct race_rank_entry {
	unsigned char name[3];	   // three character name
	unsigned char monkey_type; // observed vals=0x0,0x01,0x02,0x03
	unsigned char time[3];	   // ie, {0x63,0x3b,0x63} is 99'59"99
	unsigned char unk_00;	   // always seems to be 0x00?
}__attribute__((__packed__));

struct race_rank_table {
	struct race_rank_entry jungle_circuit[6];
	struct race_rank_entry aqua_offroad[6];
	struct race_rank_entry frozen_highway[6];
	struct race_rank_entry sky_downtown[6];
	struct race_rank_entry pipe_warp_tunnel[6];
	struct race_rank_entry speed_desert[6];
}__attribute__((__packed__));

/* Target party-game tables -------------------- */
struct target_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char score[2];
	unsigned char unk_0000[2]; //always seems to be {0x00,0x00}
}__attribute__((__packed__));

struct target_rank_table {
	struct target_rank_entry five_rounds[5];
	struct target_rank_entry ten_rounds[5];
	struct target_rank_entry fifteen_rounds[5];
}__attribute__((__packed__));

/* Bowling mini-game score tables -------------- */
struct bowling_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char score[2];
}__attribute__((__packed__));

struct bowling_rank_table {
	struct bowling_rank_entry normal_mode[5];
	struct bowling_rank_entry challenge_mode[5];
}__attribute__((__packed__));

/* Golf mini-game score tables ----------------- */
struct golf_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char out[2]; 
	unsigned char in[2]; //total score (in+out) computed in-game?
}__attribute__((__packed__));

struct golf_rank_table {
	struct golf_rank_entry eighteen_holes[5];
}__attribute__((__packed__));

/* Main game score tables ---------------------- */
struct main_rank_entry {
	unsigned char name[3];
	unsigned char unk_a; // always zero in default savedata, floor#?
	unsigned char score[4]; // unclear if this is 4 bytes?

	/* Unclear what these eight bytes are in the default savedata.
	 * All entries are { 0x00, 0x0a, 0xfc, 0x80, 0xff, 0x00, 0xff, 0x00}
	 *					    floor#?
	 */
	unsigned char unk_b[8];
}__attribute__((__packed__));

struct main_game_table {
	struct main_rank_entry beginner_mode[5];
	struct main_rank_entry advanced_mode[5];
	struct main_rank_entry expert_mode[5];
}__attribute__((__packed__));

struct party_game_table {
	struct race_rank_table	 monkey_race;
	struct target_rank_table monkey_target;
}__attribute__((__packed__));

struct mini_game_table {
	struct bowling_rank_table monkey_bowling;
	struct golf_rank_table    monkey_golf;
}__attribute__((__packed__));


/* Top-level gamedata structure, to fit inside `struct smb_gamedata`.
 * Seems to be something like this:
 *	0x40-byte block  --
 *	0x6c-byte block  -- 
 *	0x1fc-byte block -- (party + mini-game scores)
 *	0x20-byte block
 *	0xf0-byte block  -- (main game rankings)
 *	(zero padding until EOF)
 */
struct smb_gamedata_cont {
	unsigned char unk_7[0x40]; 
	unsigned char unk_8[0x6c]; 
	struct party_game_table party_game_scores;
	struct mini_game_table  mini_game_scores;

	// 32 bytes of unknown data. 
	// low-order byte for play points is at 0x17 (dunno how wide it is)
	unsigned char unk_9[0x20]; 
	struct main_game_table  main_game_scores;
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
