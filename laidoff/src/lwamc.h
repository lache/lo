#pragma once

typedef struct _LWPOSTURE LWPOSTURE;
typedef struct _LWASF LWASF;

typedef struct _LWAMC {
    int frame_count;
    LWPOSTURE* postures;
} LWAMC;

LWAMC* lwamc_new_from_file(const char* filename, LWASF* asf);
void lwamc_delete(LWAMC* amc);
