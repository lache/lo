// https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm

typedef int OutCode;

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)
// ASSUME THAT xmax, xmin, ymax and ymin are global constants.

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

static OutCode ComputeOutCode(float xmin,
                              float ymin,
                              float xmax,
                              float ymax,
                              float x,
                              float y) {
    OutCode code;

    code = INSIDE;          // initialised as being inside of [[clip window]]

    if (x < xmin)           // to the left of clip window
        code |= LEFT;
    else if (x > xmax)      // to the right of clip window
        code |= RIGHT;
    if (y < ymin)           // below the clip window
        code |= BOTTOM;
    else if (y > ymax)      // above the clip window
        code |= TOP;

    return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (xmin, ymin) to (xmax, ymax).
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
                            float* y1_clipped) {
    // compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
    OutCode outcode0 = ComputeOutCode(xmin,
                                      ymin,
                                      xmax,
                                      ymax,
                                      x0,
                                      y0);
    OutCode outcode1 = ComputeOutCode(xmin,
                                      ymin,
                                      xmax,
                                      ymax,
                                      x1,
                                      y1);
    int accept = 0;
    while (1) {
        if (!(outcode0 | outcode1)) {
            // bitwise OR is 0: both points inside window; trivially accept and exit loop
            accept = 1;
            break;
        } else if (outcode0 & outcode1) {
            // bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
            // or BOTTOM), so both must be outside window; exit loop (accept is false)
            break;
        } else {
            // failed both tests, so calculate the line segment to clip
            // from an outside point to an intersection with clip edge
            float x, y;

            // At least one endpoint is outside the clip rectangle; pick it.
            OutCode outcodeOut = outcode0 ? outcode0 : outcode1;

            // Now find the intersection point;
            // use formulas:
            //   slope = (y1 - y0) / (x1 - x0)
            //   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
            //   y = y0 + slope * (xm - x0), where xm is xmin or xmax
            // No need to worry about divide-by-zero because, in each case, the
            // outcode bit being tested guarantees the denominator is non-zero
            if (outcodeOut & TOP) {           // point is above the clip window
                x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
                y = ymax;
            } else if (outcodeOut & BOTTOM) { // point is below the clip window
                x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
                y = ymin;
            } else if (outcodeOut & RIGHT) {  // point is to the right of clip window
                y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
                x = xmax;
            } else if (outcodeOut & LEFT) {   // point is to the left of clip window
                y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
                x = xmin;
            }

            // Now we move outside point to intersection point to clip
            // and get ready for next pass.
            if (outcodeOut == outcode0) {
                x0 = x;
                y0 = y;
                outcode0 = ComputeOutCode(xmin,
                                          ymin,
                                          xmax,
                                          ymax,
                                          x0,
                                          y0);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = ComputeOutCode(xmin,
                                          ymin,
                                          xmax,
                                          ymax,
                                          x1,
                                          y1);
            }
        }
    }
    if (accept) {
        // Following functions are left for implementation by user based on
        // their platform (OpenGL/graphics.h etc.)
        //DrawRectangle(xmin, ymin, xmax, ymax);
        //LineSegment(x0, y0, x1, y1);
        *x0_clipped = x0;
        *y0_clipped = y0;
        *x1_clipped = x1;
        *y1_clipped = y1;
    } else {
        *x0_clipped = 0;
        *y0_clipped = 0;
        *x1_clipped = 0;
        *y1_clipped = 0;
    }
    return accept;
}
