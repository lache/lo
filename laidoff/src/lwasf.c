#include "lwasf.h"
#include <stdio.h>
#include <stdlib.h>
#include "lwlog.h"
#include "lwstrtok_r.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

static void remove_rn(char* line) {
    if (line[strlen(line) - 1] == '\r')
        line[strlen(line) - 1] = 0;
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = 0;
}

static int name2idx(const LWASF* asf, const char* name) {
    int i = 0;
    while (strcmp(asf->bones[i].name, name) != 0 && i++ < MAX_BONES_IN_ASF_FILE);
    return asf->bones[i].idx;
}

static LWASFBONE* get_bone(LWASF* asf, int bone_idx) {
    int i;
    for (i = 0; i < asf->bone_count; i++) {
        if (asf->bones[i].idx == bone_idx) {
            return &asf->bones[i];
        }
    }
    return 0;
}

static int set_children_and_sibling(LWASF* asf, int parent_idx, LWASFBONE* child) {
    LWASFBONE* parent;
    parent = get_bone(asf, parent_idx);
    if (parent == 0) {
        LOGE("Cannot find bone with index %d!", parent_idx);
        return 0;
    } else {
        if (parent->child == 0) {
            parent->child = child;
        } else {
            parent = parent->child;
            while (parent->sibling != 0) {
                parent = parent->sibling;
            }
            parent->sibling = child;
        }
        return 1;
    }
}

static void rotation_z(double r[][4], double a) {
    a = a * M_PI / 180.;
    r[0][0] = cos(a); r[0][1] = -sin(a); r[0][2] = 0; r[0][3] = 0;
    r[1][0] = sin(a); r[1][1] = cos(a);  r[1][2] = 0; r[1][3] = 0;
    r[2][0] = 0;      r[2][1] = 0;       r[2][2] = 1; r[2][3] = 0;
    r[3][0] = 0;      r[3][1] = 0;       r[3][2] = 0; r[3][3] = 1;
}

static void rotation_y(double r[][4], double a) {
    a = a * M_PI / 180.;
    r[0][0] = cos(a);  r[0][1] = 0;       r[0][2] = sin(a); r[0][3] = 0;
    r[1][0] = 0;       r[1][1] = 1;       r[1][2] = 0;      r[1][3] = 0;
    r[2][0] = -sin(a); r[2][1] = 0;       r[2][2] = cos(a); r[2][3] = 0;
    r[3][0] = 0;       r[3][1] = 0;       r[3][2] = 0;      r[3][3] = 1;
}

static void rotation_x(double r[][4], double a) {
    a = a * M_PI / 180.;
    r[0][0] = 1;       r[0][1] = 0;       r[0][2] = 0;       r[0][3] = 0;
    r[1][0] = 0;       r[1][1] = cos(a);  r[1][2] = -sin(a); r[1][3] = 0;
    r[2][0] = 0;       r[2][1] = sin(a);  r[2][2] = cos(a);  r[2][3] = 0;
    r[3][0] = 0;       r[3][1] = 0;       r[3][2] = 0;       r[3][3] = 1;
}

// out = M * (x, y, z) [MATRIX * VECTOR]; M assumed to be affine -- last row 0 0 0 1
static void matrix_transform_affine(double M[4][4], double x, double y, double z, double out[3]) {
    out[0] = M[0][0] * x + M[0][1] * y + M[0][2] * z + M[0][3];
    out[1] = M[1][0] * x + M[1][1] * y + M[1][2] * z + M[1][3];
    out[2] = M[2][0] * x + M[2][1] * y + M[2][2] * z + M[2][3];
}

// v = Rz(rz) Ry(ry) Rx(rx) v
static void rotate_vector_xyz(double* v, double rx, double ry, double rz) {
    double Rx[4][4], Ry[4][4], Rz[4][4];
    rotation_z(Rz, rz);
    rotation_y(Ry, ry);
    rotation_x(Rx, rx);
    matrix_transform_affine(Rz, v[0], v[1], v[2], v);
    matrix_transform_affine(Ry, v[0], v[1], v[2], v);
    matrix_transform_affine(Rx, v[0], v[1], v[2], v);
}

static void rotate_bone_dir_to_local(LWASF* asf) {
    int i;
    for (i = 1; i < asf->bone_count; i++) {
        rotate_vector_xyz(asf->bones[i].dir,
                          -asf->bones[i].axis_x,
                          -asf->bones[i].axis_y,
                          -asf->bones[i].axis_z);
    }
}

static void compute_rotation_to_parent(LWASF* asf) {
    int i;
    double Rx[4][4], Ry[4][4], Rz[4][4], tmp[4][4], tmp2[4][4];
    rotation_z(Rz, asf->bones[0].axis_z);
    rotation_y(Ry, asf->bones[0].axis_y);
    rotation_x(Rx, asf->bones[0].axis_x);
}

LWASF* lwasf_new_from_file(const char* filename) {
    LWASF* asf;
    char line[2048];
    char keyword[256];
    FILE* file;
    char* token;
    char* token_last;
    int i, x;
    int done = 0;
    double length;
    int parent = 0;

    file = fopen(filename, "r");
    if (file == 0) {
        LOGE("lwasf: file '%s' cannot be read.", filename);
        return 0;
    }

    // skip to ':bonedata' line
    while (fgets(line, sizeof(line), file)) {
        remove_rn(line);
        sscanf(line, "%s", keyword);
        LOGI("Line: %s", line);
        if (strcmp(keyword, ":bonedata") == 0)
            break;
    }

    // new LWASF instance
    asf = (LWASF*)calloc(1, sizeof(LWASF));
    // init root bone
    strcpy(asf->bones[0].name, "root");
    asf->bones[0].dofo[0] = 4;
    asf->bones[0].dofo[1] = 5;
    asf->bones[0].dofo[2] = 6;
    asf->bones[0].dofo[3] = 1;
    asf->bones[0].dofo[4] = 2;
    asf->bones[0].dofo[5] = 3;
    asf->bones[0].dofo[6] = 0; // sentinel
    asf->bones[0].idx = 0;
    asf->bones[0].length = 0.05f;
    asf->bones[0].dof = 6;
    asf->bones[0].dofrx = 1;
    asf->bones[0].dofry = 1;
    asf->bones[0].dofrz = 1;
    asf->bones[0].doftx = 1;
    asf->bones[0].dofty = 1;
    asf->bones[0].doftz = 1;
    asf->root_bone = &asf->bones[0];
    asf->bone_count++;
    asf->moving_bone_count++;
    // read bones other than root bone
    for (i = 1; (!done) && (i < MAX_BONES_IN_ASF_FILE); i++) {
        // increment bone count first expecting there is another one
        asf->bone_count++;
        asf->moving_bone_count++;
        // for each bone
        while (1) {
            if (fgets(line, sizeof(line), file) == 0) {
                LOGE("Unexpectedly terminated ASF file.");
                goto FAIL;
            }
            remove_rn(line);
            sscanf(line, "%s", keyword);
            if (strcmp(keyword, "end") == 0) {
                break;
            }
            if (strcmp(keyword, ":hierarchy") == 0) {
                asf->bone_count--;
                asf->moving_bone_count--;
                // proper exit flag
                done = 1;
                break;
            }
            if (strcmp(keyword, "id") == 0) {
                asf->bones[i].idx = asf->bone_count - 1;
            }
            if (strcmp(keyword, "name") == 0) {
                sscanf(line, "%s %s", keyword, asf->bones[i].name);
            }
            if (strcmp(keyword, "direction") == 0) {
                sscanf(line, "%s %lf %lf %lf",
                       keyword,
                       &asf->bones[i].dir[0],
                       &asf->bones[i].dir[1],
                       &asf->bones[i].dir[2]);
            }
            if (strcmp(keyword, "length") == 0) {
                sscanf(line, "%s %lf", keyword, &length);
            }
            if (strcmp(keyword, "axis") == 0) {
                sscanf(line, "%s %lf %lf %lf",
                       keyword,
                       &asf->bones[i].axis_x,
                       &asf->bones[i].axis_y,
                       &asf->bones[i].axis_z);
            }
            if (strcmp(keyword, "dof") == 0) {
                for (token = lwstrtok_r(line, " ", &token_last); token; token = lwstrtok_r(0, " ", &token_last)) {
                    int tdof = asf->bones[i].dof;
                    /****/ if (strcmp(token, "rx") == 0) {
                        asf->bones[i].dofrx = 1, asf->bones[i].dofo[tdof] = 1;
                    } else if (strcmp(token, "ry") == 0) {
                        asf->bones[i].dofry = 1, asf->bones[i].dofo[tdof] = 2;
                    } else if (strcmp(token, "rz") == 0) {
                        asf->bones[i].dofrz = 1, asf->bones[i].dofo[tdof] = 3;
                    } else if (strcmp(token, "tx") == 0) {
                        asf->bones[i].doftx = 1, asf->bones[i].dofo[tdof] = 4;
                    } else if (strcmp(token, "ty") == 0) {
                        asf->bones[i].dofty = 1, asf->bones[i].dofo[tdof] = 5;
                    } else if (strcmp(token, "tz") == 0) {
                        asf->bones[i].doftz = 1, asf->bones[i].dofo[tdof] = 6;
                    } else if (strcmp(token, "l") == 0) {
                        asf->bones[i].doftl = 1, asf->bones[i].dofo[tdof] = 7;
                    } else if (strcmp(token, "dof") == 0) {
                        continue;
                    } else {
                        LOGE("asf: Unknown DOF");
                    }
                    asf->bones[i].dof++;
                    asf->bones[i].dofo[asf->bones[i].dof] = 0; // sentinel
                }
                LOGI("Bone %d DOF", i);
                for (x = 0; x < 7 && asf->bones[i].dofo[x] != 0; x++) {
                    LOGI("%d", asf->bones[i].dofo[x]);
                }
            }
        }
        if (asf->bones[i].dofrx == 0 && asf->bones[i].dofry && asf->bones[i].dofrz) {
            asf->moving_bone_count--;
        }
        asf->bones[i].length = length;
    }
    LOGI("%d bones loaded", asf->bone_count);
    
    // hierarchy
    // skip begin line
    if (fgets(line, sizeof(line), file) == 0) {
        LOGE("Unexpectedly terminated ASF file. (expecting hierarchy data)");
        goto FAIL;
    }
    remove_rn(line);
    while (1) {
        if (fgets(line, sizeof(line), file) == 0) {
            LOGE("Unexpectedly terminated ASF file. (expecting hierarchy data)");
            goto FAIL;
        }
        remove_rn(line);
        sscanf(line, "%s", keyword);
        if (strcmp(keyword, "end") == 0) {
            break;
        } else {
            for (i = 0, token = lwstrtok_r(line, " ", &token_last);
                 token;
                 i++, token = lwstrtok_r(0, " ", &token_last)) {
                if (i == 0) {
                    parent = name2idx(asf, token);
                } else {
                    set_children_and_sibling(asf, parent, &asf->bones[name2idx(asf, token)]);
                }
            }
        }
    }
    fclose(file), file = 0;

    rotate_bone_dir_to_local(asf);

    compute_rotation_to_parent(asf);

    return asf;
FAIL:
    free(asf), asf = 0;
    return 0;
}
