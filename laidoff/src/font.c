#include <stdio.h>
#include <stdlib.h>
#include "lwlog.h"
#include "font.h"
#include "file.h"

// http://www.angelcode.com/products/bmfont/doc/file_format.html#bin

#pragma pack(push, 1)
typedef struct
{
	u8 identifier[3];
	u8 version;
} BMF_HEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	u8 blockTypeIdentifier;
	u32 blockSize;

	s16 fontSize;
	u8 bitField;
	u8 charSet;
	u16 stretchH;
	u8 aa;
	u8 paddingUp;
	u8 paddingRight;
	u8 paddingDown;
	u8 paddingLeft;
	u8 spacingHoriz;
	u8 spacingVert;
	u8 outline;
	//const char* fontName;
} BMF_INFO_BLOCK;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	u8 blockTypeIdentifier;
	u32 blockSize;

	u16 lineHeight;
	u16 base;
	u16 scaleW;
	u16 scaleH;
	u16 pages;
	u8 bitField;
	u8 alphaChnl;
	u8 redChnl;
	u8 greenChnl;
	u8 blueChnl;
} BMF_COMMON_BLOCK;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	u8 blockTypeIdentifier;
	u32 blockSize;

	//const char* pageNames;
} BMF_PAGE_BLOCK;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	u8 blockTypeIdentifier;
	u32 blockSize;
} BMF_CHAR_BLOCK;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	u8 blockTypeIdentifier;
	u32 blockSize;

	u32 first;
	u32 second;
	s16 amount;
} BMF_KERNING_PAIRS;
#pragma pack(pop)

typedef struct
{
	BMF_HEADER* header;
	BMF_INFO_BLOCK* info_block;
	BMF_COMMON_BLOCK* common_block;
	BMF_PAGE_BLOCK* page_block;
	BMF_CHAR_BLOCK* char_block;
	BMF_CHAR* c;

	char* d;
} LWFNT;

void* load_fnt(const char* filename)
{
	size_t size = 0;
	char* d = create_binary_from_file(filename, &size);
    if (!d) {
        LOGE("load_fnt: create_binary_from_file null, filename: %s", filename);
        return 0;
    }
	int cursor = 0;

	BMF_HEADER* bmf_header = (BMF_HEADER*)d;

	cursor += sizeof(BMF_HEADER);

	BMF_INFO_BLOCK* bmf_info_block = (BMF_INFO_BLOCK*)(d + cursor);

	//const char* font_name = (const char*)bmf_info_block + sizeof(BMF_INFO_BLOCK);

	cursor += sizeof(u8) + sizeof(u32) + bmf_info_block->blockSize;

	BMF_COMMON_BLOCK* bmf_common_block = (BMF_COMMON_BLOCK*)(d + cursor);

	cursor += sizeof(u8) + sizeof(u32) + bmf_common_block->blockSize;

	BMF_PAGE_BLOCK* bmf_page_block = (BMF_PAGE_BLOCK*)(d + cursor);

	//const char* first_page_name = (const char*)bmf_page_block + sizeof(BMF_PAGE_BLOCK);

	cursor += sizeof(u8) + sizeof(u32) + bmf_page_block->blockSize;

	BMF_CHAR_BLOCK* bmf_char_block = (BMF_CHAR_BLOCK*)(d + cursor);

	cursor += sizeof(u8) + sizeof(u32);

	BMF_CHAR* bmf_char = (BMF_CHAR*)(d + cursor);

	LWFNT* pFnt = malloc(sizeof(LWFNT));

	pFnt->header = bmf_header;
	pFnt->info_block = bmf_info_block;
	pFnt->common_block = bmf_common_block;
	pFnt->page_block = bmf_page_block;
	pFnt->char_block = bmf_char_block;
	pFnt->c = bmf_char;
	pFnt->d = d;

	return pFnt;
}

const BMF_CHAR* font_binary_search_char(const void* d, u32 id)
{
	const LWFNT* pFnt = d;
	const int char_count = pFnt->char_block->blockSize / sizeof(BMF_CHAR);
	if (char_count <= 0)
	{
		return 0;
	}

	int beg = 0;
	int end = char_count - 1;
	
	do
	{
		int mid = (beg + end) / 2;

		if (pFnt->c[mid].id == id)
		{
			return &pFnt->c[mid];
		}
		else if (pFnt->c[mid].id < id)
		{
			beg = mid + 1;
		}
		else
		{
			end = mid - 1;
		}
	} while (beg <= end);

	return 0;
}

int font_get_base(const void* d)
{
	const LWFNT* pFnt = d;
	return pFnt->common_block->base;
}

int font_get_line_height(const void* d)
{
	const LWFNT* pFnt = d;
	return pFnt->common_block->lineHeight;
}

void release_font(void* d)
{
	LWFNT* pFnt = d;
	release_binary(pFnt->d);
	free(d);
}
