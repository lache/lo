#include "lwasf.h"
#include <stdio.h>
#include <stdlib.h>
#include "lwlog.h"
#include "lwstrtok_r.h"
#include <math.h>
#include <string.h> // ios strlen()
#include "file.h"
#include "lwcharptrstream.h"
#ifndef M_PI
#define M_PI 3.14159265
#endif

static void remove_rn(char* line) {
    if (line[strlen(line) - 1] == '\r')
        line[strlen(line) - 1] = 0;
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = 0;
}

int lwasf_name2idx(const LWASF* asf, const char* name) {
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

// r is row-major matarix
static void rotation_z(double r[][4], double a) {
    a = a * M_PI / 180.;
    r[0][0] = cos(a); r[0][1] = -sin(a); r[0][2] = 0; r[0][3] = 0;
    r[1][0] = sin(a); r[1][1] = cos(a);  r[1][2] = 0; r[1][3] = 0;
    r[2][0] = 0;      r[2][1] = 0;       r[2][2] = 1; r[2][3] = 0;
    r[3][0] = 0;      r[3][1] = 0;       r[3][2] = 0; r[3][3] = 1;
}

// r is row-major matarix
static void rotation_y(double r[][4], double a) {
    a = a * M_PI / 180.;
    r[0][0] = cos(a);  r[0][1] = 0;       r[0][2] = sin(a); r[0][3] = 0;
    r[1][0] = 0;       r[1][1] = 1;       r[1][2] = 0;      r[1][3] = 0;
    r[2][0] = -sin(a); r[2][1] = 0;       r[2][2] = cos(a); r[2][3] = 0;
    r[3][0] = 0;       r[3][1] = 0;       r[3][2] = 0;      r[3][3] = 1;
}

// r is row-major matarix
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

// v = (Rz(rz) Ry(ry) Rx(rx))^(-1) v
static void inverse_rotate_vector_xyz(double* v, double rx, double ry, double rz) {
    double Rx[4][4], Ry[4][4], Rz[4][4];
    rotation_z(Rz, -rz);
    rotation_y(Ry, -ry);
    rotation_x(Rx, -rx);
    matrix_transform_affine(Rz, v[0], v[1], v[2], v);
    matrix_transform_affine(Ry, v[0], v[1], v[2], v);
    matrix_transform_affine(Rx, v[0], v[1], v[2], v);
}

static void rotate_bone_dir_to_local_coordinate(LWASF* asf) {
    int i;
    // exclude root bone
    for (i = 1; i < asf->bone_count; i++) {
        inverse_rotate_vector_xyz(asf->bones[i].dir,
                                  asf->bones[i].axis_x,
                                  asf->bones[i].axis_y,
                                  asf->bones[i].axis_z);
    }
}

static void matrix_mult(double a[4][4], double b[4][4], double c[4][4]) {
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            c[i][j] = 0;
            for (k = 0; k < 4; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

static void matrix_transpose(double a[4][4], double b[4][4]) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            b[i][j] = a[i][j];
        }
    }
}

static void compute_coordinate_transform_from_child_to_parent(LWASFBONE* parent, LWASFBONE* child) {
    if (parent && child) {
        double Rx[4][4], Ry[4][4], Rz[4][4], tmp[4][4], tmp1[4][4], tmp2[4][4];
        // tmp1 := (coordinate transform matrix from inertia frame to parent)^(-1)
        //       = (coordinate transform matrix from parent to inertia frame)
        //       = (RzRyRx)^(-1)
        //       = Rx^(-1)Ry^(-1)Rz^(-1)
        rotation_z(Rz, -parent->axis_z);
        rotation_y(Ry, -parent->axis_y);
        rotation_x(Rx, -parent->axis_x);
        matrix_mult(Rx, Ry, tmp);
        matrix_mult(tmp, Rz, tmp1);
        // tmp2 := (coordinate transform matrix from inertia frome to child)
        rotation_z(Rz, child->axis_z);
        rotation_y(Ry, child->axis_y);
        rotation_x(Rx, child->axis_x);
        matrix_mult(Rz, Ry, tmp);
        matrix_mult(tmp, Rx, tmp2);
        // tmp := tmp1 * tmp2
        matrix_mult(tmp1, tmp2, tmp);
        // child->rot_parent_current := (coordinate transform matrix from child to parent)
        //                            = (coordinate transform matrix from child to inertia frame) * (coordinate transform matrix from inertia frame to parent)
        //                            = ((coordinate transform matrix from parent to inertia frame) * (coordinate transform matrix from inertia frame to child))^T
        //                            = (coordinate transform matrix from inertia frame to child)^T * (coordinate transform matrix from parent to inertia frame)^T
        //                            = tmp2^T * tmp1^T
        //                            = (tmp1 * tmp2)^T
        matrix_transpose(tmp, child->rot_parent_current);
    } else {
        LOGE("asf: Unexpected NULL");
    }
}

static void compute_coordinate_transform_from_child_to_parent_all(LWASF* asf) {
    int i;
    double Rx[4][4], Ry[4][4], Rz[4][4], tmp[4][4], tmp2[4][4];
    // root first
    // tmp2 := o_R_A
    rotation_z(Rz, asf->bones[0].axis_z);
    rotation_y(Ry, asf->bones[0].axis_y);
    rotation_x(Rx, asf->bones[0].axis_x);
    matrix_mult(Rz, Ry, tmp);
    matrix_mult(tmp, Rx, tmp2);
    // rot_parent_current := A_R_o
    matrix_transpose(tmp2, asf->bones[0].rot_parent_current);
    // for all other child bones
    for (i = 0; i < asf->bone_count; i++) {
        if (asf->bones[i].child) {
            LWASFBONE* sibling;
            compute_coordinate_transform_from_child_to_parent(&asf->bones[i], asf->bones[i].child);
            sibling = asf->bones[i].child->sibling;
            while (sibling) {
                compute_coordinate_transform_from_child_to_parent(&asf->bones[i], sibling);
                sibling = sibling->sibling;
            }
        }
    }
}
// Use this parameter to adjust the size of the skeleton. The default value is 0.06.
// This creates a human skeleton of 1.7 m in height (approximately)
//#define MOCAP_SCALE 0.06
#define MOCAP_SCALE 0.06

static LWASF* load_asf(const char* filename) {
    LWASF* asf;
    char line[2048];
    char keyword[256];
    char* token;
    char* token_last;
    int i, x;
    int done = 0;
    double length = 1;
    int parent = 0;
    LWCHARPTRSTREAM char_ptr_stream;

    lwcharptrstream_init(&char_ptr_stream, create_string_from_file(filename));
    if (char_ptr_stream.length == 0) {
        LOGE("lwasf: file '%s' cannot be read.", filename);
        return 0;
    }

    // skip to ':bonedata' line
    while (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line))) {
        remove_rn(line);
        sscanf(line, "%s", keyword);
        //LOGI("Line: %s", line);
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
            if (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line)) == 0) {
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
            // bone direction is in world coordinate system
            // this will be transformed in local coordinate system later
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
                        asf->bones[i].dofrx = 1;
                        asf->bones[i].dofo[tdof] = 1;
                    } else if (strcmp(token, "ry") == 0) {
                        asf->bones[i].dofry = 1;
                        asf->bones[i].dofo[tdof] = 2;
                    } else if (strcmp(token, "rz") == 0) {
                        asf->bones[i].dofrz = 1;
                        asf->bones[i].dofo[tdof] = 3;
                    } else if (strcmp(token, "tx") == 0) {
                        asf->bones[i].doftx = 1;
                        asf->bones[i].dofo[tdof] = 4;
                    } else if (strcmp(token, "ty") == 0) {
                        asf->bones[i].dofty = 1;
                        asf->bones[i].dofo[tdof] = 5;
                    } else if (strcmp(token, "tz") == 0) {
                        asf->bones[i].doftz = 1;
                        asf->bones[i].dofo[tdof] = 6;
                    } else if (strcmp(token, "l") == 0) {
                        asf->bones[i].doftl = 1;
                        asf->bones[i].dofo[tdof] = 7;
                    } else if (strcmp(token, "dof") == 0) {
                        continue;
                    } else {
                        LOGE("asf: Unknown DOF");
                    }
                    asf->bones[i].dof++;
                    asf->bones[i].dofo[asf->bones[i].dof] = 0; // sentinel
                }
                //LOGI("Bone %d DOF", i);
                for (x = 0; x < 7 && asf->bones[i].dofo[x] != 0; x++) {
                    //LOGI("%d", asf->bones[i].dofo[x]);
                }
            }
        }
        if (asf->bones[i].dofrx == 0 && asf->bones[i].dofry && asf->bones[i].dofrz) {
            asf->moving_bone_count--;
        }
        asf->bones[i].length = length * MOCAP_SCALE;
    }
    LOGI("%d bones loaded", asf->bone_count);

    // hierarchy
    // skip begin line
    if (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line)) == 0) {
        LOGE("Unexpectedly terminated ASF file. (expecting hierarchy data)");
        goto FAIL;
    }
    remove_rn(line);
    while (1) {
        if (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line)) == 0) {
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
                    parent = lwasf_name2idx(asf, token);
                } else {
                    set_children_and_sibling(asf, parent, &asf->bones[lwasf_name2idx(asf, token)]);
                }
            }
        }
    }
    lwcharptrstream_deinit(&char_ptr_stream);
    return asf;
FAIL:
    lwcharptrstream_deinit(&char_ptr_stream);
    free(asf); asf = 0;
    return 0;
}

LWASF* lwasf_new_from_file(const char* filename) {
    LWASF* asf;
    asf = load_asf(filename);
    if (asf) {
        rotate_bone_dir_to_local_coordinate(asf);
        compute_coordinate_transform_from_child_to_parent_all(asf);
    }
    return asf;
}

void lwasf_delete(LWASF* asf) {
    free(asf);
}

void lwasf_enable_all_rotational_dofs(LWASF* asf) {
    int i;
    for (i = 0; i < asf->bone_count; i++) {
        if (asf->bones[i].dof == 0) {
            continue;
        }
        if (asf->bones[i].dofrx) {
            asf->bones[i].dofrx = 1;
            asf->bones[i].rx = 0;
            asf->bones[i].dof++;
            asf->bones[i].dofo[asf->bones[i].dof - 1] = 1;
            asf->bones[i].dofo[asf->bones[i].dof] = 0;
        }
        if (asf->bones[i].dofry) {
            asf->bones[i].dofry = 1;
            asf->bones[i].ry = 0;
            asf->bones[i].dof++;
            asf->bones[i].dofo[asf->bones[i].dof - 1] = 2;
            asf->bones[i].dofo[asf->bones[i].dof] = 0;
        }
        if (asf->bones[i].dofrz) {
            asf->bones[i].dofrz = 1;
            asf->bones[i].rz = 0;
            asf->bones[i].dof++;
            asf->bones[i].dofo[asf->bones[i].dof - 1] = 3;
            asf->bones[i].dofo[asf->bones[i].dof] = 0;
        }
    }
}

LWASFBONE* lwasf_bone(LWASF* asf, int index) {
    return &asf->bones[index];
}
