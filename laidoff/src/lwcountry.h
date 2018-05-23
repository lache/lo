#pragma once

typedef struct _LWCOUNTRY {
    char code[4];
    char name[64];
} LWCOUNTRY;

typedef struct _LWCOUNTRYARRAY {
    int count;
    LWCOUNTRY* first;
} LWCOUNTRYARRAY;
