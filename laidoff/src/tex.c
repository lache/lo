#include "lwgl.h"
#include "lwcontext.h"
#include "tex.h"

#define LW_GRID_WIDTH 64
#define LW_GRID_HEIGHT 64
#define LW_GRID_CHANNELS 3

#define LW_GRADIENT_WIDTH 128
#define LW_GRADIENT_SLOPE_WIDTH 8
#define LW_GRADIENT_HEIGHT 1
#define LW_GRADIENT_CHANNELS 4

void load_tex_program(struct _LWCONTEXT *pLwc) {
    glGenTextures(MAX_TEX_PROGRAMMED, pLwc->tex_programmed);

    for (int i = 0; i < MAX_TEX_PROGRAMMED; i++) {
        glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[i]);

        if (i == LPT_GRID) {
            unsigned char grid[LW_GRID_HEIGHT][LW_GRID_WIDTH][LW_GRID_CHANNELS];
            for (int gy = 0; gy < LW_GRID_HEIGHT; gy++) {
                for (int gx = 0; gx < LW_GRID_WIDTH; gx++) {
                    const int colored = (
                        ((gx / (LW_GRID_WIDTH / 2)) % 2 + (gy / (LW_GRID_HEIGHT / 2)) % 2) % 2);

                    if (colored) {
                        grid[gy][gx][0] = 232;
                        grid[gy][gx][1] = 193;
                        grid[gy][gx][2] = 50;
                    } else {
                        grid[gy][gx][0] = 255;
                        grid[gy][gx][1] = 136;
                        grid[gy][gx][2] = 102;
                    }
                }
            }
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         LW_GRID_WIDTH,
                         LW_GRID_HEIGHT,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_RED) {
            unsigned char grid[] = { 255, 0, 0 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_GREEN) {
            unsigned char grid[] = { 0, 255, 0 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_BLUE) {
            unsigned char grid[] = { 0, 0, 255 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_EXP_COLOR) {
            unsigned char grid[] = {
                (int)(EXP_COLOR[0] * 255),
                (int)(EXP_COLOR[1] * 255),
                (int)(EXP_COLOR[2] * 255),
            };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB, GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_YELLOW) {
            unsigned char grid[] = { 255, 255, 0 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_GRAY) {
            unsigned char grid[] = { 128, 128, 128 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_BLACK) {
            unsigned char grid[] = { 0, 0, 0 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1,
                         1,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_TRANSPARENT) {
            unsigned char grid[] = { 0, 0, 0, 0 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         1,
                         1,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_SOLID_WHITE_WITH_ALPHA) {
            unsigned char grid[] = { 255, 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         1,
                         1,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_DIR_PAD) {
            unsigned char pattern[16][16] = {
                {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
                {0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0},
                {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
                {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
                {1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                {1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1},

                {1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1},
                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                {1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
                {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
                {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
                {0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0},
                {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
            };
            unsigned char grid[16 * 4][16 * 4][4];
            for (int py = 0; py < 16; py++) {
                for (int px = 0; px < 16; px++) {
                    const int colored = pattern[py][px];
                    unsigned char r, g, b, a;
                    if (colored) {
                        r = g = b = 0;
                        a = 255 / 3;
                    } else {
                        r = g = b = 0xff;
                        a = 0;
                    }

                    for (int sgy = 0; sgy < 4; sgy++) {
                        for (int sgx = 0; sgx < 4; sgx++) {
                            int gy = 4 * py + sgy;
                            int gx = 4 * px + sgx;

                            grid[gy][gx][0] = r;
                            grid[gy][gx][1] = g;
                            grid[gy][gx][2] = b;
                            grid[gy][gx][3] = a;
                        }
                    }
                }
            }

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         16 * 4,
                         16 * 4,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         grid);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }

        if (i == LPT_BOTH_END_GRADIENT_HORIZONTAL) {
            unsigned char grad[LW_GRADIENT_HEIGHT][LW_GRADIENT_WIDTH][LW_GRADIENT_CHANNELS];
            for (int gy = 0; gy < LW_GRADIENT_HEIGHT; gy++) {
                for (int gx = 0; gx < LW_GRADIENT_WIDTH; gx++) {

                    int v = 0;
                    if (gx < LW_GRADIENT_SLOPE_WIDTH) {
                        v = (int)(255 * (1.0f / LW_GRADIENT_SLOPE_WIDTH) * gx);
                    } else if (LW_GRADIENT_SLOPE_WIDTH <= gx && gx < LW_GRADIENT_WIDTH - LW_GRADIENT_SLOPE_WIDTH) {
                        v = 255;
                    } else {
                        v = (int)(255 * (-1.0f / LW_GRADIENT_SLOPE_WIDTH) * (gx - LW_GRADIENT_WIDTH + 1));
                    }

                    grad[gy][gx][0] = 0;
                    grad[gy][gx][1] = 0;
                    grad[gy][gx][2] = 0;
                    grad[gy][gx][3] = v & 0xff;
                }
            }
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         LW_GRADIENT_WIDTH,
                         LW_GRADIENT_HEIGHT,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         grad);
            glGenerateMipmap(GL_TEXTURE_2D);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}
