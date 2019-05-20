#include "precompiled.hpp"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/bignum.h"

static int g_initialized = 0;
static mbedtls_entropy_context entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;
static mbedtls_mpi * RR;

static void init_random() {
    if (g_initialized)
        return;

    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg_ctx);

    unsigned char hotBits[128] = {
   82, 42, 71, 87, 124, 241, 30, 1, 54, 239, 240, 121, 89, 9, 151, 11, 60,
   226, 142, 47, 115, 157, 100, 126, 242, 132, 46, 12, 56, 197, 194, 76,
   198, 122, 90, 241, 255, 43, 120, 209, 69, 21, 195, 212, 100, 251, 18,
   111, 30, 238, 24, 199, 238, 236, 138, 225, 45, 15, 42, 83, 114, 132,
   165, 141, 32, 185, 167, 100, 131, 23, 236, 9, 11, 51, 130, 136, 97, 161,
   36, 174, 129, 234, 2, 54, 119, 184, 70, 103, 118, 109, 122, 15, 24, 23,
   166, 203, 102, 160, 77, 100, 17, 4, 132, 138, 215, 204, 109, 245, 122,
   9, 184, 89, 70, 247, 125, 97, 213, 240, 85, 243, 91, 226, 127, 64, 136,
   37, 154, 232
    };

    mbedtls_ctr_drbg_seed(
        &ctr_drbg_ctx,
        mbedtls_entropy_func,
        &entropy_ctx,
        hotBits,
        128
    );

    RR = (mbedtls_mpi *)malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(RR);
    g_initialized = 1;

}

int srp_alloc_random_bytes(unsigned char ** b, int len_b) {

    init_random();

    *b = (unsigned char*)malloc((size_t)len_b);
    if (*b == 0) {
        return -1;
    }
    return mbedtls_ctr_drbg_random(&ctr_drbg_ctx, *b, len_b);
}


// Based on https://stackoverflow.com/a/23898449/266720
void srp_unhexify(const char * str, unsigned char ** b, int * len_b) {
    int  pos;
    int  idx0;
    int  idx1;

    // mapping of ASCII characters to hex values
    const uint8_t hashmap[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
        0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
        0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
    };

    int slen = static_cast<int>(strlen(str));
    int blen = slen / 2;
    unsigned char* bb = (unsigned char*)malloc(blen);
    for (pos = 0; ((pos < (blen * 2)) && (pos < slen)); pos += 2) {
        idx0 = ((uint8_t)str[pos + 0] & 0x1F) ^ 0x10;
        idx1 = ((uint8_t)str[pos + 1] & 0x1F) ^ 0x10;
        bb[pos / 2] = (uint8_t)(hashmap[idx0] << 4) | hashmap[idx1];
    };
    *b = bb;
    *len_b = blen;
}
