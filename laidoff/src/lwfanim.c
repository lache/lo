#include "lwfanim.h"
#include "lwcontext.h"
#include "file.h"

void load_fanim(LWCONTEXT* pLwc, const char* filename, LWFANIM* fanim) {
    size_t file_size = 0;
    char* data = create_binary_from_file(filename, &file_size);
    fanim->data = data;
    fanim->total_cell_count = *(int*)data;
    fanim->total_frame_count = *((int*)data + 1);
    fanim->anim_key = (LWFANIMKEY*)((int*)data + 2);
    //release_binary(data); -- should not be released at this time.
}
