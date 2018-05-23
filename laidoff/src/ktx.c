#include <stdio.h>
#include <string.h>

#include "lwgl.h"
#include "image.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "ktx.h"
#include "file.h"

typedef unsigned int UInt32;

unsigned int swap_4_bytes(unsigned int num);

// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
#pragma pack(push, 1)
typedef struct {
    char identifier[12];
    char endianness[4];
    UInt32 glType;
    UInt32 glTypeSize;
    UInt32 glFormat;
    UInt32 glInternalFormat;
    UInt32 glBaseInternalFormat;
    UInt32 pixelWidth;
    UInt32 pixelHeight;
    UInt32 pixelDepth;
    UInt32 numberOfArrayElements;
    UInt32 numberOfFaces;
    UInt32 numberOfMipmapLevels;
    UInt32 bytesOfKeyValueData;
} LWKTX;
#pragma pack(pop)

unsigned char FileIdentifier[] =
{
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

#ifdef GL_ETC1_RGB8_OES
static const int etc1_support = 1;
static const int GL_ETC1_RGB8_OES_VALUE = GL_ETC1_RGB8_OES;
#else
static const int etc1_support = 0;
static const int GL_ETC1_RGB8_OES_VALUE = -1;
#endif

int load_ktx_hw_or_sw_memory(const char* b, int* width, int* height, const char* tex_atlas_filename) {
    LWKTX* pKtx = (LWKTX*)b;
    if (memcmp(pKtx->identifier, FileIdentifier, 12) != 0) {
        // Invalid KTX data
        return -1;
    }

    int big_endian = pKtx->endianness[0] == 0x04
        && pKtx->endianness[1] == 0x03
        && pKtx->endianness[2] == 0x02
        && pKtx->endianness[3] == 0x01;

    UInt32 internal_format = big_endian ? swap_4_bytes(pKtx->glInternalFormat) : pKtx->glInternalFormat;

    if (internal_format != 0x00008D64 /*GL_ETC1_RGB8_OES*/) {
        // Not supported encoding format
        return -2;
    }

    short extended_width = big_endian ? swap_4_bytes(pKtx->pixelWidth) : pKtx->pixelWidth;
    short extended_height = big_endian ? swap_4_bytes(pKtx->pixelHeight) : pKtx->pixelHeight;
    short mip_levels = big_endian ? swap_4_bytes(pKtx->numberOfMipmapLevels) : pKtx->numberOfMipmapLevels;

    *width = extended_width;
    *height = extended_height;

    GLenum error_enum;

    const char* data_offset = b + sizeof(LWKTX) + pKtx->bytesOfKeyValueData;

    for (int i = 0; i < mip_levels; i++) {
        UInt32 data_size = big_endian ? swap_4_bytes(*(UInt32*)data_offset) : *(UInt32*)data_offset;
        const char* d = data_offset + 4;

        const short mip_width = extended_width >> i;
        const short mip_height = extended_height >> i;

        if (etc1_support == 0) {
            char* decoded_rgb_data = load_software_decode_etc1_rgb(mip_width, mip_height, d);
            glGetError(); // clear previous errors
            glTexImage2D(GL_TEXTURE_2D,
                         i,
                         GL_RGB,
                         mip_width,
                         mip_height,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         decoded_rgb_data);
            release_software_decode_etc1_rgb(decoded_rgb_data);
        } else {
            glGetError(); // clear previous errors
            glCompressedTexImage2D(GL_TEXTURE_2D,
                                   i,
                                   GL_ETC1_RGB8_OES_VALUE,
                                   mip_width,
                                   mip_height,
                                   0,
                                   data_size,
                                   d);
        }

        error_enum = glGetError();
        LOGI("gl[Compressed]TexImage2D (ETC1) %s level %d result (%dx%d): %d",
             tex_atlas_filename,
             i,
             mip_width,
             mip_height,
             error_enum);
        if (error_enum) {
            LOGE("GL Call Error!: %d", error_enum);
        }

        data_offset += 4 + data_size;
    }
    return 0;
}

int load_ktx_hw_or_sw(const char* tex_atlas_filename, int* width, int* height) {
    size_t file_size = 0;
    char *b = create_binary_from_file(tex_atlas_filename, &file_size);
    if (!b) {
        LOGE("load_ktx_hw_or_sw: create_binary_from_file null, filename %s", tex_atlas_filename);
        return -5;
    }
    int code = load_ktx_hw_or_sw_memory(b, width, height, tex_atlas_filename);
    if (code != 0) {
        LOGE("load_ktx_hw_or_sw_memory: failed! return code = %d", code);
        return code;
    }
    release_binary(b);
    return 0;
}
