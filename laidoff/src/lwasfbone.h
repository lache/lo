#pragma once

typedef struct _LWASFBONE LWASFBONE;

typedef struct _LWASFBONE {
    LWASFBONE* sibling;
    LWASFBONE* child;
    int idx;
    double dir[3];
    double length;
    double axis_x, axis_y, axis_z;
    int dof;
    int dofrx, dofry, dofrz;
    int doftx, dofty, doftz;
    int doftl;
    char name[256];
    double rot_parent_current[4][4];
    double rx, ry, rz;
    double tx, ty, tz;
    double tl;
    int dofo[8];
} LWASFBONE;
