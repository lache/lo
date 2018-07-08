#include "unicode.h"

static const unsigned int offsetsFromUTF8[6] =
{
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char trailingBytesForUTF8[256] = 
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5,
    5
};

/* conversions without error checking
only works for valid UTF-8, i.e. no 5- or 6-byte sequences
srcsz = source size in bytes, or -1 if 0-terminated
sz = dest size in # of wide characters

returns # characters converted
dest will always be L'\0'-terminated, even if there isn't enough room
for all the characters.
if sz = srcsz+1 (i.e. 4*srcsz+4 bytes), there will always be enough space.
*/
int u8_toucs(unsigned int* dest, int sz, const char *src, int srcsz)
{
    unsigned int ch;
    const char *src_end = src + srcsz;
    int nb;
    int i = 0;

    while (i < sz - 1) {
        nb = trailingBytesForUTF8[(unsigned char)*src];
        if (srcsz == -1) {
            if (*src == 0)
                goto done_toucs;
        }
        else {
            if (src + nb >= src_end)
                goto done_toucs;
        }
        ch = 0;
        switch (nb) {
            /* these fall through deliberately */
        case 3:
            ch += (unsigned char)*src++;
            ch <<= 6;
        case 2:
            ch += (unsigned char)*src++;
            ch <<= 6;
        case 1:
            ch += (unsigned char)*src++;
            ch <<= 6;
        case 0:
            ch += (unsigned char)*src++;
        }
        ch -= offsetsFromUTF8[nb];
        dest[i++] = ch;
    }
done_toucs:
    dest[i] = 0;
    return i;
}
