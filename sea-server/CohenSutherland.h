#pragma once
#if __cplusplus
extern "C" {
#endif
int CohenSutherlandLineClip(double xmin,
                            double ymin,
                            double xmax,
                            double ymax,
                            double x0,
                            double y0,
                            double x1,
                            double y1,
                            double* x0_clipped,
                            double* y0_clipped,
                            double* x1_clipped,
                            double* y1_clipped);
#if __cplusplus
};
#endif
