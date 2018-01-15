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

enum minigame_type {
	BILLIARDS = 0x01,
	BOWLING = 0x02,
	GOLF = 0x04,
};

/* This is on the header of every .GCI file */
struct dentry {
	uint8_t gamecode[4];
	uint8_t makercode[2];
	uint8_t unused_a;
	uint8_t bi_flags;
	char filename[0x20];
	uint8_t modtime[4];
	uint8_t image_offset[4];
	uint8_t icon_fmt[2];
	uint8_t anim_speed[2];
	uint8_t permissions;
	uint8_t copy_counter;
	uint8_t first_block[2];
	uint16_t block_count;
	uint8_t unused_b[2];
	uint32_t comments_addr;
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
 * Zero-length array at the end should be replaced after
 * we've finished articulating the actual format. */
struct smb_gamedata {
	unsigned char checksum[2];
	uint16_t gamedata_version; // I think?
	unsigned char banner[0x5800];
	unsigned char comments[COMMENT_SIZE];
	unsigned char unk_6[4];
	unsigned char savedata[0]; //eventually turn into struct
}__attribute__((__packed__));


/* Entries in each of the high score ranking tables */
struct race_rank_entry {
	unsigned char name[3];	   // three character name
	unsigned char monkey_type; // 0x00 - 0x03
	unsigned char time[3];	   // ie, {0x63,0x3b,0x63} is 99'59"99
	unsigned char unk_00;	   // always seems to be 0x00?
}__attribute__((__packed__));

struct target_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char score[2];
	unsigned char unk_0000[2]; //always seems to be {0x00,0x00}
}__attribute__((__packed__));

struct bowling_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char score[2];
}__attribute__((__packed__));

struct golf_rank_entry {
	unsigned char name[3];
	unsigned char monkey_type;
	unsigned char out_score[2];
	unsigned char in_score[2];
}__attribute__((__packed__));

struct main_rank_entry {
	unsigned char name[3];
	unsigned char unk_a; // always zero in default savedata
	unsigned char score[4]; // unclear if this is 4 bytes?

	/* Unclear what these eight bytes are in the default savedata.
	 * All entries are { 0x00, 0x0a, 0xfc, 0x80, 0xff, 0x00, 0xff, 0x00}
	 */
	unsigned char unk_b[8];
}__attribute__((__packed__));

/* The ranking tables are just arrays of the entries
 * described above */
struct main_game_table {
	struct main_rank_entry beginner_mode[5];
	struct main_rank_entry advanced_mode[5];
	struct main_rank_entry expert_mode[5];
}__attribute__((__packed__));

struct party_game_table {
	struct race_rank_entry jungle_circuit[6];
	struct race_rank_entry aqua_offroad[6];
	struct race_rank_entry frozen_highway[6];
	struct race_rank_entry sky_downtown[6];
	struct race_rank_entry pipe_warp_tunnel[6];
	struct race_rank_entry speed_desert[6];
	struct target_rank_entry five_rounds[5];
	struct target_rank_entry ten_rounds[5];
	struct target_rank_entry fifteen_rounds[5];
}__attribute__((__packed__));

struct mini_game_table {
	struct bowling_rank_entry normal_mode[5];
	struct bowling_rank_entry challenge_mode[5];
	struct golf_rank_entry eighteen_holes[5];
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
	//unsigned char gamedata_block_1[0x40];
	float p1_calibration[4];
	float p2_calibration[4];
	float p3_calibration[4];
	float p4_calibration[4];

	// gamedata_block_2
	struct {
		unsigned char gameselect_cursor_idx; // unvalidated
		unsigned char maingame_modeselect_cursor_idx; // unvalidated

		unsigned char unk_86_87[2];

		// P1 - P4 character selection cursor
		unsigned char character_select_idx[4]; // unvalidated

		unsigned char unk_8c_8d[2]; // 50 5a
		unsigned char maingame_difficulty_cursor_idx; // 00 ;unvalidated
		unsigned char maingame_difficulty_cursor_idx_2; // 00 ;unvalidated
		unsigned char unk_90; // 03
		unsigned char unk_91_95[5]; // 33 05 03 01 00

		unsigned char unk_96_97[2]; // 0000 (i think padding)

		uint16_t unk_98_9a[2]; // pair stored with u32 at 0xc4
		uint16_t unk_9c_aa[8]; // each pair of uint16_t is stored with a u32 from 0xb0

		unsigned char unk_ac_af[4]; // 00000000 (padding?)

		uint32_t unk_b0_bc[4];
		uint32_t unk_c0_c4[2];
		uint16_t unk_c8_ca[2]; // stored with u32 at 0xc0
		uint32_t unk_cc;
		uint32_t unk_d0_dc[4];
		uint32_t unk_e0;

		// Party Games->Monkey race->OneCourse Race
		unsigned char jungle_circuit_laps;
		unsigned char aqua_offroad_laps;
		unsigned char frozen_highway_laps;
		unsigned char sky_downtop_laps;
		unsigned char pipe_warp_tunnel_laps;
		unsigned char speed_desert_laps;

		unsigned char partygames_monkeyrace_course_select_idx; // unvalidated
		unsigned char partygames_monkeyrace_cursor_idx; // unvalidated

		unsigned char unk_ec; // unvalidated input (SEGA logo if !=0x00)
		unsigned char unk_ed; // 0f    (r3 in call to 800b6224)
		unsigned char partygames_monkeytarget_rounds; // 0a
		unsigned char unk_ef; // 00
	};


	struct party_game_table party_game_scores;
	struct mini_game_table  mini_game_scores;

	// starts at offset 0x2ec; 0x20 bytes wide
	struct {
		uint32_t unk_2ec; // 00 00 00 00
		
		unsigned char unk_2f0; // 00
		unsigned char unk_2f1; // 00
		unsigned char unk_2f2; // 00
		unsigned char unk_2f3; // 03
		unsigned char unk_2f4; // 00 ;unvalidated

		unsigned char minigame_billiards_num_sets; // 03

		/* 0x00 - LVL1; 0x01 - LVL2; ... */
		unsigned char minigame_billiards_com_level; // 02;unvalidated
		unsigned char minigame_billiards_cursor_idx; // 00;unvalidated

		unsigned char unk_2f8;
		unsigned char unk_2f9;
		unsigned char unk_2fa;
		unsigned char unk_2fb;

		unsigned char minigame_golf_cursor_idx; // 00

		/* 0x00 - 18 holes; 0x01 - OUT; 0x02 - IN */
		unsigned char minigame_golf_holes_idx; // 00

		unsigned char unk_2fe; // 00

		/* u8 bitmask? (maybe it's a u16?)
		 * (1 << 0) = billiards;  (1 << 1) = bowling
		 * (1 << 2) = golf 
		 * Dunno what the other bits do (yet) */
		unsigned char minigames_unlocked; // 00

		int32_t play_points; // (remember to convert to BE)
		uint32_t unk_304;

		unsigned char unk_308_30b[4]; // zero padding?
	};

	struct main_game_table  main_game_scores;
	uint32_t unk_3fc; // 00 00 00 00 (zero padding?)
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
