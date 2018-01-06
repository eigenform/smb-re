#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "smb_compression.h"
#include "smb.h"

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


/* Replace a gamedata GCI's contents with data from /dev/urandom,
 * starting immediately after the two checksum bytes. 
 *
 * Modifying the `struct dentry` header on GCI files seems to make
 * SMB complain about data corruption :^( 
 */
int main(int argc, char* argv[]){
	if (argc < 2) {
		printf("usage: random-gci <output GCI>\n");
		return -1; 
	}

	FILE *f,*r;

	/* Just going to consider this a valid-enough `struct dentry` to use
	 * as a template (unless we happen to find that it affects something, 
	 * or unless we're specifically trying to corrupt it) to make valid GCIs 
	 */
	struct dentry header = { 
		{0x47, 0x4d, 0x42, 0x45},	//gamecode
		{0x38,0x50},			//makercode	
		0xff,				//unused_a
		0x02,				//bi_flags
		"super_monkey_ball.sys",	//filename
		{0x21,0xe1,0x23,0x80},		//modtime
		{0x00,0x00,0x00,0x04},		//image_offset
		{0xaa,0xaa},			//icon_fmt
		{0x15,0x55},			//anim_speed
		0x04,				//permissions
		0x00,				//copy_counter
		{0x00,0x4e},			//first_block
		{0x00,0x03},			//block_count
		{0xff,0xff},			//unused_b
		{0x00,0x00,0x58,0x04}		//comments_addr
	};


	struct gci_gamedata *input = calloc(1, GCI_GAMEDATA_SIZE*2);
	printf("[*] Output: %s\n", argv[1]);

	/* Read some randomness; sorry for wasting your precious entropy */
	unsigned char rand[0x10000];
	f = fopen("/dev/urandom", "rb");
	fread(&rand, 1, 0x10000, f);
	fclose(f);

	r = fopen(argv[1], "wb");
	if (r == NULL) {
		printf("[!] Couldn't open %s for writing\n", argv[2]);
		return -1;
	}

	/* Copy over our template dentry */
	memcpy(&input->dentry,&header,sizeof(struct dentry));

	/* gamedata_version is usually set to {0x00,0x16}. When zeroed out, 
	 * the game complains "The gamedata version is incorrect" on boot.
	 */
	unsigned char gamedata_version[2] = {0x00,0x16};
	memcpy(&input->data.gamedata_version, &gamedata_version, 2);

	/* Fill with Random data starting at the banner */
	memcpy(&input->data.banner, &rand, 0x6000);

	/* Fix checksum before writing */
	fix_checksum((char*)input, GCI_GAMEDATA_SIZE, GAME_DATA);

	/* Write output GCI */
	fwrite(input, 1, GCI_GAMEDATA_SIZE, r);
	fclose(r);
	printf("[*] Wrote new GCI to %s\n", argv[2]);
	free(input);
	return 0;
}
