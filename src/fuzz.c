#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "smb_compression.h"
#include "smb.h"

#define GCI_SIZE 0x4040
#define GCI_GAMEDATA_SIZE 0x6040

 /* Pulled this function from good-ol' StackExchange, see:
  * questions/7775991/how-to-get-hexdump-of-a-structure-data */
void hexdump(char *desc, void *addr, int len)
{
	unsigned char *pc = (unsigned char*)addr;
	unsigned char buff[17];
	int i;

	if (desc != NULL) printf ("%s:\n", desc);
	if (len == 0)
	{
		printf("  ZERO LENGTH\n");
		return;
	}
	if (len < 0)
	{
		printf("  NEGATIVE LENGTH: %i\n",len);
		return;
	}

	for (i = 0; i < len; i++)
	{
		if ((i % 16) == 0)
		{
			if (i != 0)
				printf("  %s\n", buff);
			printf("  %04x ", i);
		}
		
		printf(" %02x", pc[i]);
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	while ((i % 16) != 0)
	{
		printf("   ");
		i++;
	}
	
	printf("  %s\n", buff);
}

/* Take some input GCI and produce some modified output. Intended to take
 * "default" gamedata as input. To generate the default gamedata GCI, load an
 * empty memory card, and navigate to Options->Game Data->Save. This should
 * produce an 8P-GMBE-super_monkey_ball.sys.gci.
 *
 * I generally use this like:
 *
 *	$ # Adjust some value, then
 *	$ make && ./fuzz 8P-GMBE-super_monkey_ball.sys.gci /tmp/fuzz.gci \
 *	$      && cp /tmp/fuzz.gci ~/.local/share/dolphin-emu/GC/USE/Card\ A/
 *
 * Kinda clunky, yes. There's no particular reason this is in C. A Python 
 * script may be more suitable if you prefer. You could also just use bash to 
 * fuzz GCI input, ie:
 *
 *	$ # Write some value to the character-selection index
 *	$ printf '\x3b' | dd of=/tmp/replay-charsel-fuzz.gci bs=1 \
 *	$	seek=$((0x209e)) count=1 conv=notrunc 
 *	$
 *	$ # bobjrsenior's utility
 *	$ ./fix-checksum /tmp/replay-charsel-fuzz.gci
 *	$
 *	$ # Move to GCI folder and test
 *	$ cp /tmp/replay-charsel-fuzz.gci \
 *	$	~/.local/share/dolphin-emu/GC/USA/Card\ A/ 
 * 
 * Even better, you could probably look at `800a4f04`, `80012170`, and find 
 * where fields are resident in memory, then just hack up some setup with 
 * savestates and modify these values in Dolphin.
 */
int main(int argc, char* argv[]){
	if (argc < 3) {
		printf("usage: fuzz <input GCI> <output GCI>\n");
		return -1;
	}

	FILE *f, *r;
	unsigned char buf[MAX_SIZE*2];
	unsigned char res[MAX_SIZE*2];
	struct gci_gamedata *input = calloc(1, GCI_GAMEDATA_SIZE*2);
	printf("[*] GCI   : %s\n", argv[1]);
	printf("[*] Output: %s\n", argv[2]);

	/* Read input GCI */
	f = fopen(argv[1], "rb");
	if (f == NULL) {
		printf("[!] Couldn't open %s\n", argv[1]);
		return -1;
	}
	fread(input, 1, GCI_GAMEDATA_SIZE, f);
	fclose(f);

	/* Oh no, not my entropy! */
	unsigned char rand[0x10000];
	f = fopen("/dev/urandom", "rb");
	fread(&rand, 1, 0x10000, f);
	fclose(f);

	r = fopen(argv[2], "wb");
	if (r == NULL) {
		printf("[!] Couldn't open %s for writing\n", argv[2]);
		return -1;
	}

	struct smb_gamedata_cont *data =
		(struct smb_gamedata_cont*)&input->data.savedata;

	/* In case you want to fuzz the `struct dentry` associated with some
	 * GCI, these are dentry properties that we apparently need to set
	 * to prevent the game from recognizing the file as invalid.
	 * Haven't explored these much.
	 */

	uint8_t gc[4] = { 0x47, 0x4d, 0x42, 0x45 };
	uint8_t mc[2] = { 0x38, 0x50 };
	memcpy(&input->dentry.gamecode, &gc, 4);
	memcpy(&input->dentry.makercode, &mc, 2);
	input->dentry.comments_addr = b32(0x00005804);
	input->dentry.block_count = b16(0x0003); // limit to 3 blocks
	input->data.gamedata_version = b16(0x0016);
	strncpy(input->dentry.filename, "super_monkey_ball.sys", 22);

	/* Make sure we normalize fields known-to-crash on invalid inputs.
	 * These fields appear to mostly be indicies or offsets into tables
	 * that are not validated before handling.
	 *
	 * Fields representing menu indicies typically crash for particular
	 * values when navigating into the associated gamemode.
	 * ie. indicies for minigame menu selection crash upon starting the
	 * relevant minigame.
	 */

	data->gameselect_cursor_idx = 0x00;
	data->maingame_modeselect_cursor_idx = 0x00;
	data->character_select_idx[0] = 0x00;
	data->character_select_idx[1] = 0x01;
	data->character_select_idx[2] = 0x02;
	data->character_select_idx[3] = 0x03;
	data->maingame_difficulty_cursor_idx = 0x00;
	data->maingame_difficulty_cursor_idx_2 = 0x00;
	data->partygames_monkeyrace_course_select_idx = 0x00;
	data->partygames_monkeyrace_cursor_idx = 0x00;
	data->unk_ec = 0x00; //SEGA logo crash

	data->unk_2f0 = 0x01;
	data->unk_2f1 = 0x01;
	data->unk_2f2 = 0x01;
	data->unk_2f3 = 0x03;
	data->unk_2f4 = 0x01;

	data->minigame_billiards_com_level = 0x02; //crash after charsel
	data->minigame_billiards_cursor_idx = 0x00; // crash on billiards

	data->minigame_golf_cursor_idx = 0x00;
	data->minigame_golf_holes_idx = 0x00; // 0x00-0x02 valid?

	// Unlock all minigames for access to other code paths
	data->minigames_unlocked = BILLIARDS | BOWLING | GOLF;

	// Set play points high enough for access to other code paths
	data->play_points = b32(0x7FFFFFFF);


	/* Fix GCI checksum here before writing a file */
	fix_checksum((char*)input, GCI_GAMEDATA_SIZE, GAME_DATA);

	/* Write output GCI */
	fwrite(input, 1, GCI_GAMEDATA_SIZE, r);
	fclose(r);
	printf("[*] Wrote new GCI to %s\n", argv[2]);
	free(input);
	return 0;
}
