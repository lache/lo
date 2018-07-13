#include <jni.h>
#include <malloc.h>
#include <cwchar>
#include "lwbitmapcontext.h"
#ifdef WIN32
// TODO
#else
#include <android/log.h>
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "native-activity", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "native-activity", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))
#define LOGA(...) ((void)__android_log_print(ANDROID_LOG_ASSERT, "native-activity", __VA_ARGS__))
#endif
#include "lwime.h"
#include "lwttl.h"
#include "lwcontext.h"

extern "C" void init_register_asset();
extern "C" void register_asset(const char* asset_path, int start_offset, int length);
extern "C" void set_apk_path(const char* apk_path);
extern "C" void set_files_path(const char* files_path);
extern "C" void set_package_version(const char* package_version);

void request_void_string_command(const char* command_name, const char* param1);
int request_int_string_command(const char* command_name, const char* param1);

static int tex_width_pushed_from_java;
static int tex_height_pushed_from_java;
static char* tex_data_pushed_from_java[3]; // atlas count
LWCONTEXT* pLwc;


extern "C" int init_ext_image_lib()
{
    return 0;
}

extern "C" void create_image(const char* filename, LWBITMAPCONTEXT* pBitmapContext, int tex_atlas_index)
{
    LOGI("create_image %s calling loadBitmap Java function... tex_atlas_index=%d", filename, tex_atlas_index);
    int load_bitmap_result = request_int_string_command("loadBitmap", filename);
    LOGI("create_image %s result %d pixels (= width x height) tex_atlas_index=%d", filename, load_bitmap_result, tex_atlas_index);

    pBitmapContext->width = tex_width_pushed_from_java;
    pBitmapContext->height = tex_height_pushed_from_java;
    pBitmapContext->data = tex_data_pushed_from_java[0];
}

extern "C" void release_image(LWBITMAPCONTEXT* pBitmapContext)
{
    free(pBitmapContext->data);
}

extern "C" JNIEXPORT int JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_pushTextureData(JNIEnv *env, jobject /* this */, jint w, jint h, jintArray data, jint tex_atlas_index) {
    int len = w*h*4;
    char *body = (char*)env->GetIntArrayElements(data, 0);


    tex_width_pushed_from_java = w;
    tex_height_pushed_from_java = h;
    tex_data_pushed_from_java[tex_atlas_index] = (char*)malloc(len);

    // This converts the ARGB data from Java into RGBA data OpenGL can use.
    for(int i = 0; i < len; i += 4)
    {
        tex_data_pushed_from_java[tex_atlas_index][i] = body[i+2];
        tex_data_pushed_from_java[tex_atlas_index][i+1] = body[i+1];
        tex_data_pushed_from_java[tex_atlas_index][i+2] = body[i];
        tex_data_pushed_from_java[tex_atlas_index][i+3] = body[i+3];
    }

    env->ReleaseIntArrayElements(data, (jint*)body, 0);

    LOGI("pushTextureData (tex_atlas_index index=%d): pushed %d bytes", tex_atlas_index, len);

    return len;
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_initRegisterAsset(JNIEnv * env, jclass cls) {
    init_register_asset();
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_registerAsset(JNIEnv * env, jclass cls, jstring assetpath, int startoffset, int length) {
    const char *buffer = env->GetStringUTFChars(assetpath, JNI_FALSE);

    register_asset(buffer, startoffset, length);

    env->ReleaseStringUTFChars(assetpath, buffer);
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_sendInputText(JNIEnv * env, jclass cls, jstring text) {
    const char *buffer = env->GetStringUTFChars(text, JNI_FALSE);

    strcpy(lw_get_text_input_for_writing(), buffer);
    lw_increase_text_input_seq();

    env->ReleaseStringUTFChars(text, buffer);
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_SignInActivity_setNickname(JNIEnv * env, jclass cls, jstring text) {
    const char *buffer = env->GetStringUTFChars(text, JNI_FALSE);

    lw_set_text_input_tag(LITI_NICKNAME);
    strcpy(lw_get_text_input_for_writing(), buffer);
    lw_increase_text_input_seq();

    env->ReleaseStringUTFChars(text, buffer);
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_sendApkPath(JNIEnv * env,
                                                                                                  jclass cls,
                                                                                                  jstring apkPath,
                                                                                                  jstring filesPath,
                                                                                                  jstring packageVersion) {
    const char* apkPathBuffer = env->GetStringUTFChars(apkPath, JNI_FALSE);
    set_apk_path(apkPathBuffer);
    env->ReleaseStringUTFChars(apkPath, apkPathBuffer);

    const char* filesPathBuffer = env->GetStringUTFChars(filesPath, JNI_FALSE);
    set_files_path(filesPathBuffer);
    env->ReleaseStringUTFChars(filesPath, filesPathBuffer);

    const char* packageVersionBuffer = env->GetStringUTFChars(packageVersion, JNI_FALSE);
    set_package_version(packageVersionBuffer);
    env->ReleaseStringUTFChars(packageVersion, packageVersionBuffer);
}

extern "C" JNIEXPORT void JNICALL Java_com_popsongremix_laidoff_LaidoffNativeActivity_sendChatInputText(JNIEnv * env, jclass cls, jstring text) {
    const char *buffer = env->GetStringUTFChars(text, JNI_FALSE);

    lwttl_udp_send_ttlchat(pLwc->ttl, lwttl_sea_udp(pLwc->ttl), buffer);

    env->ReleaseStringUTFChars(text, buffer);
}
