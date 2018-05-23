#pragma once
#if __cplusplus
extern "C" {
#endif
int CohenSutherlandLineClip(float xmin,
                            float ymin,
                            float xmax,
                            float ymax,
                            float x0,
                            float y0,
                            float x1,
                            float y1,
                            float* x0_clipped,
                            float* y0_clipped,
                            float* x1_clipped,
                            float* y1_clipped);
#if __cplusplus
};
#endif
