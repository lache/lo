#pragma once

typedef struct _LWASFBONE LWASFBONE;

typedef struct _LWASFBONE {
    LWASFBONE* sibling;
    LWASFBONE* child;
    int idx;
    // bone direction in local coordinate system
    double dir[3];
    double length;
    // bone orientation in world coordinate system;
    // 'RzRyRx' == 'coordinate transfrom from inertia frame to local coordinate system'
    double axis_x, axis_y, axis_z;
    int dof;
    int dofrx, dofry, dofrz;
    int doftx, dofty, doftz;
    int doftl;
    char name[256];
    // coordinate transform matrix from the local coordinate of this bone
    // to the local coordinate system of its parent
    double rot_parent_current[4][4];
    double rx, ry, rz;
    double tx, ty, tz;
    double tl;
    int dofo[8];
} LWASFBONE;

double lwasfbone_rot_parent_current(const LWASFBONE* bone, int r, int c);
double lwasfbone_dir(const LWASFBONE* bone, int axis);
