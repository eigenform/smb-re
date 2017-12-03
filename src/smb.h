

// The maximum size of inflated replay data
#define MAX_SIZE 0x16844

// Offset to actual savedata in GCI
#define GCI_METADATA_SIZE 0x2040

// Offset to start of deflated replay data, from start of savedata
#define HEADER_SIZE 0x58


struct region {
	unsigned char hdr[HEADER_SIZE];
	unsigned char data[0];
}__attribute__((__packed__));

struct gci {
	unsigned char dentry[GCI_METADATA_SIZE];
	struct region region;
}__attribute__((__packed__));



