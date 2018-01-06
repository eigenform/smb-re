all:
	gcc -o smb-inflate smb-inflate.c smb_compression.c
	gcc -o smb-deflate smb-deflate.c smb_compression.c
	#gcc -o fuzz fuzz.c smb_compression.c
	gcc -o random-gci random-gci.c smb_compression.c
clean:
	rm -v smb-inflate smb-deflate
