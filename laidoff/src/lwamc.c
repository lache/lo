#include "lwamc.h"
#include "lwasf.h"
#include "lwasfbone.h"
#include "lwposture.h"
#include <stdio.h>
#include "lwlog.h"
#include "lwstrtok_r.h"
#include <stdlib.h> // ios atoi()
#include <string.h> // ios strlen()

char * resolve_path(const char * filename);

static void remove_rn(char* line) {
    if (line[strlen(line) - 1] == '\r')
        line[strlen(line) - 1] = 0;
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = 0;
}

static LWAMC* load_amc(const char* filename, LWASF* asf) {
    LWAMC* amc;
    const LWASFBONE* bone;
    FILE* file;
    int n = 0;
    char line[2048];
    int bone_count;
    int moving_bone_count;
    int i, j, k;
    char* token;
    char* token_last;
    int bone_idx;
    double v;

    if (asf == 0) {
        LOGE("lwamf: ASF null.");
        return 0;
    }

    bone = asf->root_bone;
    char* resolved_filename = resolve_path(filename);
    file = fopen(resolved_filename, "r");
    free(resolved_filename); resolved_filename = 0;
    if (file == 0) {
        LOGE("lwamc: file '%s' cannot be read.", filename);
        return 0;
    }

    // count lines
    while (fgets(line, sizeof(line), file)) {
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

    fseek(file, 0, SEEK_SET);

    while (fgets(line, sizeof(line), file)) {
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
        fgets(line, sizeof(line), file);
        frame_num = atoi(line);
        posture = &amc->postures[i];
        for (j = 0; j < moving_bone_count; j++) {
            fgets(line, sizeof(line), file);
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
                        posture->bone_rotation[bone_idx][0] = v;
                        break;
                    case 2:
                        posture->bone_rotation[bone_idx][1] = v;
                        break;
                    case 3:
                        posture->bone_rotation[bone_idx][2] = v;
                        break;
                    case 4:
                        posture->bone_translation[bone_idx][0] = v;
                        break;
                    case 5:
                        posture->bone_translation[bone_idx][1] = v;
                        break;
                    case 6:
                        posture->bone_translation[bone_idx][2] = v;
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
    fclose(file), file = 0;
    LOGI("amc: %d frames loaded from '%s'.", n, filename);
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
