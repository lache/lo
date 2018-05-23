#pragma once

typedef unsigned int u32;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned char u8;

#pragma pack(push, 1)
typedef struct
{
	u32 id;
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	s16 xoffset;
	s16 yoffset;
	s16 xadvance;
	u8 page;
	u8 chnl;
} BMF_CHAR;
#pragma pack(pop)

void* load_fnt(const char* filename);
const BMF_CHAR* font_binary_search_char(const void* d, u32 id);
int font_get_base(const void* d);
int font_get_line_height(const void* d);
void release_font(void* d);
