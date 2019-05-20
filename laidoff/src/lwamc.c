#include "lwamc.h"
#include "lwasf.h"
#include "lwasfbone.h"
#include "lwposture.h"
#include <stdio.h>
#include "lwlog.h"
#include "lwstrtok_r.h"
#include <stdlib.h> // ios atoi()
#include <string.h> // ios strlen()
#include "lwcharptrstream.h"
#include "file.h"
#include "lwmacro.h"

static void remove_rn(char* line) {
    if (line[strlen(line) - 1] == '\r')
        line[strlen(line) - 1] = 0;
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = 0;
}

static LWAMC* load_amc(const char* filename, LWASF* asf) {
    LWAMC* amc;
    const LWASFBONE* bone;
    int n = 0;
    char line[2048];
    int bone_count;
    int moving_bone_count;
    int i, j, k;
    char* token;
    char* token_last;
    int bone_idx;
    double v;
    LWCHARPTRSTREAM char_ptr_stream;

    if (asf == 0) {
        LOGE("lwamf: ASF null.");
        return 0;
    }

    bone = asf->root_bone;
    lwcharptrstream_init(&char_ptr_stream, create_string_from_file(filename));
    if (char_ptr_stream.length == 0) {
        LOGE("lwamc: file '%s' cannot be read.", filename);
        return 0;
    }

    // count lines
    while (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line))) {
        remove_rn(line);
        if (strcmp(line, "") != 0) {
            n++;
        }
    }

    bone_count = asf->bone_count;
    moving_bone_count = asf->moving_bone_count;

    // 3 for ignoring header line
    // + 1 ??
    n = (n - 3) / (moving_bone_count + 1);
    
    amc = (LWAMC*)calloc(1, sizeof(LWAMC));
    amc->frame_count = n;
    amc->postures = (LWPOSTURE*)calloc(n, sizeof(LWPOSTURE));

    lwcharptrstream_reset_cursor(&char_ptr_stream);

    while (lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line))) {
        remove_rn(line);
        if (strcmp(line, ":FORCE-ALL-JOINTS-BE-3DOF") == 0) {
            lwasf_enable_all_rotational_dofs(asf);
        }
        if (strcmp(line, ":DEGREES") == 0) {
            break;
        }
    }

    for (i = 0; i < amc->frame_count; i++) {
        int frame_num;
        LWPOSTURE* posture;
        lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line));
        frame_num = atoi(line);
        posture = &amc->postures[i];
        for (j = 0; j < moving_bone_count; j++) {
            lwcharptrstream_fgets(&char_ptr_stream, line, sizeof(line));
            for (k = 0, token = lwstrtok_r(line, " ", &token_last);
                 token;
                 k++, token = lwstrtok_r(0, " ", &token_last)) {
                if (k == 0) {
                    // bone name -> bone index
                    bone_idx = lwasf_name2idx(asf, token);
                    posture->bone_rotation[bone_idx][0] = 0;
                    posture->bone_rotation[bone_idx][1] = 0;
                    posture->bone_rotation[bone_idx][2] = 0;
                } else {
                    v = atof(token);
                    switch (bone[bone_idx].dofo[k - 1]) {
                    case 0:
                        LOGE("amc: bone index %d not found %d channel", bone_idx, k - 1);
                        break;
                    case 1:
                        posture->bone_rotation[bone_idx][0] = LWDEG2RAD(v);
                        break;
                    case 2:
                        posture->bone_rotation[bone_idx][1] = LWDEG2RAD(v);
                        break;
                    case 3:
                        posture->bone_rotation[bone_idx][2] = LWDEG2RAD(v);
                        break;
                    case 4:
                        posture->bone_translation[bone_idx][0] = v * MOCAP_SCALE;
                        break;
                    case 5:
                        posture->bone_translation[bone_idx][1] = v * MOCAP_SCALE;
                        break;
                    case 6:
                        posture->bone_translation[bone_idx][2] = v * MOCAP_SCALE;
                        break;
                    case 7:
                        posture->bone_length[bone_idx] = v;
                        break;
                    }
                }
                if (strcmp(token, "root") == 0) {
                    posture->root_pos[0] = posture->bone_translation[0][0];
                    posture->root_pos[1] = posture->bone_translation[0][1];
                    posture->root_pos[2] = posture->bone_translation[0][2];
                }
            }
        }
    }
    LOGI("amc: %d frames loaded from '%s'.", n, filename);
    lwcharptrstream_deinit(&char_ptr_stream);
    return amc;
}

LWAMC* lwamc_new_from_file(const char* filename, LWASF* asf) {
    LWAMC* amc;
    amc = load_amc(filename, asf);
    if (amc) {
    }
    return amc;
}

void lwamc_delete(LWAMC* amc) {
    free(amc->postures);
    free(amc);
}

void lwamc_apply_posture(const LWAMC* amc, LWASF* asf, int frame) {
    if (amc == 0) {
        LOGEP("amc null");
        return;
    }
    if (asf == 0) {
        LOGEP("asf null");
        return;
    }
    if (frame >= amc->frame_count) {
        LOGEP("Frame out of range");
        return;
    }
    // amc->postures[frame].root_pos ???
    const LWPOSTURE* posture = &amc->postures[frame];
    for (int i = 0; i < asf->bone_count; i++) {
        asf->bones[i].tx = posture->bone_translation[i][0];
        asf->bones[i].ty = posture->bone_translation[i][1];
        asf->bones[i].tz = posture->bone_translation[i][2];
        asf->bones[i].rx = posture->bone_rotation[i][0];
        asf->bones[i].ry = posture->bone_rotation[i][1];
        asf->bones[i].rz = posture->bone_rotation[i][2];
    }
}
