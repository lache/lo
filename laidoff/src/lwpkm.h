#pragma once

#pragma pack(push, 1)
typedef struct {
	char header[4];
	char version[2];
	short data_type;
	short extended_width; // BIG ENDIAN!
	short extended_height; // BIG ENDIAN!
	short original_width; // BIG ENDIAN!
	short original_height; // BIG ENDIAN!
} LWPKM;
#pragma pack(pop)
