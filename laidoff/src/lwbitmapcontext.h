#pragma once

typedef struct _LWBITMPCONTEXT
{
    void* bitmap;
    void* lock;
    char* data;
    int width;
    int height;
} LWBITMAPCONTEXT;
