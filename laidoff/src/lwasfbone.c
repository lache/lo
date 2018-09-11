#include "lwasfbone.h"

double lwasfbone_rot_parent_current(const LWASFBONE* bone, int r, int c) {
    return bone->rot_parent_current[r][c];
}

double lwasfbone_dir(const LWASFBONE* bone, int axis) {
    return bone->dir[axis];
}
