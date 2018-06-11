#include <initializer_list>
#include <memory>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <unistd.h>
#include <android/input.h>
#include <stdlib.h>
#include <lwtimepoint.h>
#include <input.h>
#include <puckgame.h>
#include <lwtcp.h>
#include <lwime.h>
#include <string>
#include <lwttl.h>
#include "laidoff.h"
#include "lwlog.h"
#include "czmq.h"
#include "logic.h"
#include "sysmsg.h"

/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;

    int32_t width;
    int32_t height;
    struct saved_state state;

    bool app_cmd_init_window_triggered;
    bool inited;

    struct _LWCONTEXT* pLwc;

    // 라이프사이클 플래그
    int resumed;
    int focused;
    int surface_ready;
    int window_ready;
};

static void recreate_surface(engine* pEngine);

static JavaVM* s_vm_from_java;
static JavaVM* s_vm_from_cpp;
static JNIEnv* s_env_from_java;
static jobject s_obj_from_java;
static jobject s_obj_from_cpp;
static ALooper* s_looper_from_cpp;
static bool s_java_activity_created;
static int s_download_assets;

#define JAVA_NATIVE_ACTIVITY_NAME "com.popsongremix.laidoff.LaidoffNativeActivity"
#define TEXT_INPUT_ACTIVITY_NAME "com.popsongremix.laidoff.TextInputActivity"

extern "C" int lw_get_text_input_seq();
extern "C" const char* lw_get_text_input();
extern "C" void lw_set_push_token(LWCONTEXT* pLwc, int domain, const char* token);

const char* egl_get_error_string(EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
    }
    static char unknown_error_str[1024];
    sprintf(unknown_error_str, "Unknown error: %02x", error);
    return unknown_error_str;
}

jclass get_java_native_activity_class(JNIEnv* env) {
    jclass ClassNativeActivity = env->FindClass("android/app/NativeActivity");
    jmethodID getClassLoader = env->GetMethodID(ClassNativeActivity, "getClassLoader",
                                                "()Ljava/lang/ClassLoader;");

    jclass classLoader = env->FindClass("java/lang/ClassLoader");
    jmethodID findClass = env->GetMethodID(classLoader, "loadClass",
                                           "(Ljava/lang/String;)Ljava/lang/Class;");
    jobject cls = env->CallObjectMethod(s_obj_from_cpp, getClassLoader);

    jstring actionString = env->NewStringUTF(JAVA_NATIVE_ACTIVITY_NAME);
    return (jclass) env->CallObjectMethod(cls, findClass, actionString);
}

void request_void_string_command(const char* command_name, const char* param1) {
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jstring soundTypeString = env->NewStringUTF(param1);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name,
                                                              "(Ljava/lang/String;)V");
        env->CallStaticVoidMethod(s_java_native_activity_class, callMeStaticMethod,
                                  soundTypeString);
    }
    s_vm_from_cpp->DetachCurrentThread();
}

void request_void_int4_command(const char* command_name, int v1, int v2, int v3, int v4) {
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name,
                                                              "(IIII)V");
        env->CallStaticVoidMethod(s_java_native_activity_class, callMeStaticMethod,
                                  v1, v2, v3, v4);
    }
    s_vm_from_cpp->DetachCurrentThread();
}


void request_void_long_command(const char* command_name, jlong param1) {
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name, "(J)V");
        env->CallStaticVoidMethod(s_java_native_activity_class, callMeStaticMethod, param1);
    }
    s_vm_from_cpp->DetachCurrentThread();
}

int request_int_string_command(const char* command_name, const char* param1) {
    int ret = 0;
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jstring soundTypeString = env->NewStringUTF(param1);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name,
                                                              "(Ljava/lang/String;)I");
        ret = env->CallStaticIntMethod(s_java_native_activity_class, callMeStaticMethod,
                                       soundTypeString);
    }
    s_vm_from_cpp->DetachCurrentThread();
    return ret;
}

void request_sound_command(const char* command_name, const char* sound_type) {
    request_void_string_command(command_name, sound_type);
}

int request_get_int_command(const char* command_name) {
    int ret = 0;
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name, "()I");
        ret = env->CallStaticIntMethod(s_java_native_activity_class, callMeStaticMethod);
    }
    s_vm_from_cpp->DetachCurrentThread();

    return ret;
}

int request_get_boolean_command(const char* command_name) {
    int ret = 0;
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name, "()Z");
        ret = env->CallStaticBooleanMethod(s_java_native_activity_class, callMeStaticMethod);
    }
    s_vm_from_cpp->DetachCurrentThread();

    return ret;
}

void request_void_command(const char* command_name) {
    int ret = 0;
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name, "()V");
        env->CallStaticVoidMethod(s_java_native_activity_class, callMeStaticMethod);
    }
    s_vm_from_cpp->DetachCurrentThread();
}

void request_void_int_command(const char* command_name, int param1) {
    int ret = 0;
    JNIEnv* env;
    s_vm_from_cpp->AttachCurrentThread(&env, NULL);
    {
        jclass s_java_native_activity_class = get_java_native_activity_class(env);

        jmethodID callMeStaticMethod = env->GetStaticMethodID(s_java_native_activity_class,
                                                              command_name, "(I)V");
        env->CallStaticVoidMethod(s_java_native_activity_class, callMeStaticMethod, param1);
    }
    s_vm_from_cpp->DetachCurrentThread();
}

extern "C" int request_is_retryable() {
    return request_get_boolean_command("isRetryable");
}

extern "C" int request_is_boasted() {
    return request_get_boolean_command("isBoasted");
}

extern "C" void request_boast(int point) {
    request_void_int_command("boast", point);
}

extern "C" void request_on_game_over(int point) {
    request_void_int_command("onGameOver", point);
}

extern "C" void request_on_game_start() {
    request_void_command("onGameStart");
}

extern "C" int request_get_today_played_count() {
    return 0;//request_get_int_command("getTodayPlayedCount");
}

extern "C" int request_get_today_playing_limit_count() {
    return 0;//request_get_int_command("getTodayPlayingLimitCount");
}

extern "C" int request_get_highscore() {
    return 0;//request_get_int_command("getHighscore");
}

extern "C" void request_set_highscore(int highscore) {
    //return request_void_int_command("setHighscore", highscore);
}

void request_play_sound(const char* sound_type) {
    //request_sound_command("playSound", sound_type);
}

void request_stop_sound(const char* sound_type) {
    //request_sound_command("stopSound", sound_type);
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,  // Request opengl ES2.0
                              EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT,
                              EGL_BLUE_SIZE,
                              5,
                              EGL_GREEN_SIZE,
                              6,
                              EGL_RED_SIZE,
                              5,
                              EGL_DEPTH_SIZE,
                              16,
                              EGL_NONE};
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);

    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION,
                                      2,  // Request OpenGL ES2.0
                                      EGL_NONE};

    context = eglCreateContext(display, config, NULL, context_attribs);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    engine->surface_ready = 1;

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    int min_swap_interval, max_swap_interval;
    eglGetConfigAttrib(display, config, EGL_MIN_SWAP_INTERVAL, &min_swap_interval);
    eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL, &max_swap_interval);

    // Calling eglSwapInterval has no meaning on Android?
    // (https://groups.google.com/forum/#!topic/android-developers/HvMZRcp3pt0)
    //eglSwapInterval(display, 1);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->config = config;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    LOGI("Surface width: %d", engine->width);
    LOGI("Surface height: %d", engine->height);

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }

    if (!engine->surface_ready || !engine->focused || !engine->resumed || !engine->window_ready) {
        return;
    }

    if (engine->pLwc) {
        lwc_prerender_mutable_context(engine->pLwc);
        lwc_render(engine->pLwc);
    }

    GLboolean swap_result = eglSwapBuffers(engine->display, engine->surface);
    GLint swap_error = eglGetError();

    if (swap_result != EGL_TRUE) {
        LOGI("Swap Failed!");
        LOGI("eglGetError() = %d", swap_error);
        LOGI("eglGetError description = %s", egl_get_error_string(swap_error));

        if (swap_error == EGL_BAD_SURFACE) {
            // EGL 서피스 문제면 서피스만 새로 만들어보자...
            engine->pLwc->window_width = 1;
            engine->pLwc->window_height = 1;
            recreate_surface(engine);
        } else if (swap_error == EGL_BAD_NATIVE_WINDOW) {
            // 재초기화 위해서 `inited` 플래그 내림
            engine->inited = false;
        } else {
            LOGE("Unknown swap failure reason: %d", swap_error);
            abort();
        }
    }
}

static void recreate_surface(engine* engine) {
    // 'RE'-create surface request called before 'initial' surface is ever created.
    //if (engine->surface_ready == 0) {
    //    return;
    //}
    EGLint w, h;
    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &w);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &h);
    if (engine->pLwc && w == engine->pLwc->window_width && h == engine->pLwc->window_height) {
        LOGI("recreate_surface: skipped since there is no size change...");
        return;
    }

    engine->surface_ready = 0;
    if (engine->display && engine->config && engine->app && engine->app->window) {
        if (engine->surface != EGL_NO_SURFACE) {
            // eglDestroySurface() call below makes
            // infinite 'Emulator: Draw context is NULL' errors
            // on emulators... comment it for now
            eglDestroySurface(engine->display, engine->surface);
        }

        engine->surface = eglCreateWindowSurface(engine->display,
                                                 engine->config,
                                                 engine->app->window,
                                                 NULL);

        if (eglMakeCurrent(engine->display,
                           engine->surface,
                           engine->surface,
                           engine->context) == EGL_FALSE) {
            LOGW("Unable to eglMakeCurrent!!");
        } else {
            eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &w);
            eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &h);
            if (w > 0 && w < 5000 && h > 0 && h < 5000) {
                engine->width = w;
                engine->height = h;
				lw_set_viewport_size(engine->pLwc, w, h);
                lw_set_window_size(engine->pLwc, w, h);
                LOGI("Change surface (width x height) to (%d x %d)", w, h);
            } else {
                LOGE("Surface recreated but size query result is not valid: w=%d, h=%d", w, h);
            }
            engine->surface_ready = 1;
        }
    } else {
        LOGE("APP_CMD_CONFIG_CHANGED but surface cannot be recreated.");
    }
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }

    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*) app->userData;
    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_MOTION:
            switch (AInputEvent_getSource(event)) {
                case AINPUT_SOURCE_TOUCHSCREEN:
                    int pointer_count = AMotionEvent_getPointerCount(event);
                    //LOGI("Pointer count *** %d", pointer_count);
                    int action = AKeyEvent_getAction(event);
                    int motion_event_action = action & AMOTION_EVENT_ACTION_MASK;
                    int pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

                    const float xpos = AMotionEvent_getX(event, pointer_index);
                    const float ypos = AMotionEvent_getY(event, pointer_index);
                    //LOGI("Pointer #%d", pointer_index);
                    const int pointer_id = AMotionEvent_getPointerId(event, pointer_index);

                    int width = engine->width;
                    int height = engine->height;

                    float normalized_xpos = (float) (2 * (xpos / width - 0.5));
                    float normalized_ypos = (float) (1.0 - 2 * (ypos / height));

                    const int verbose_log = 0;
                    const int verbose_move_log = 0;

                    switch (motion_event_action) {
                        case AMOTION_EVENT_ACTION_DOWN:
                            if (verbose_log) {
                                LOGI("[%d] AMOTION_EVENT_ACTION_DOWN [%d] %.2f %.2f", pointer_count,
                                     pointer_id, xpos, ypos);
                            }
                            lw_trigger_touch(engine->pLwc, normalized_xpos, normalized_ypos,
                                             pointer_id);
                            lw_trigger_mouse_press(engine->pLwc, normalized_xpos, normalized_ypos,
                                                   pointer_id);
                            break;
                        case AMOTION_EVENT_ACTION_UP:
                            if (verbose_log) {
                                LOGI("[%d] AMOTION_EVENT_ACTION_UP [%d] %.2f %.2f", pointer_count,
                                     pointer_id, xpos, ypos);
                            }
                            lw_trigger_mouse_release(engine->pLwc, normalized_xpos, normalized_ypos,
                                                     pointer_id);
                            break;
                        case AMOTION_EVENT_ACTION_MOVE: {
                            for (size_t i = 0; i < pointer_count; i++) {
                                const int move_pointer_id = AMotionEvent_getPointerId(event, i);
                                const float move_xpos = AMotionEvent_getX(event, i);
                                const float move_ypos = AMotionEvent_getY(event, i);
                                float move_normalized_xpos = (float) (2 *
                                                                      (move_xpos / width - 0.5));
                                float move_normalized_ypos = (float) (1.0 -
                                                                      2 * (move_ypos / height));
                                if (verbose_move_log) {
                                    LOGI("[%d] AMOTION_EVENT_ACTION_MOVE [%d] %.2f %.2f",
                                         pointer_count, move_pointer_id, move_xpos, move_ypos);
                                }
                                lw_trigger_mouse_move(engine->pLwc, move_normalized_xpos,
                                                      move_normalized_ypos, move_pointer_id);
                            }
                            break;
                        }
                        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
                            if (verbose_log) {
                                LOGI("[%d] AMOTION_EVENT_ACTION_POINTER_DOWN [%d] %.2f %.2f",
                                     pointer_count, pointer_id, xpos, ypos);
                            }
                            lw_trigger_mouse_press(engine->pLwc, normalized_xpos, normalized_ypos,
                                                   pointer_id);
                            break;
                        }
                        case AMOTION_EVENT_ACTION_POINTER_UP: {
                            if (verbose_log) {
                                LOGI("[%d] AMOTION_EVENT_ACTION_POINTER_UP [%d] %.2f %.2f",
                                     pointer_count, pointer_id, xpos, ypos);
                            }
                            lw_trigger_mouse_release(engine->pLwc, normalized_xpos, normalized_ypos,
                                                     pointer_id);
                            break;
                        }
                        case AMOTION_EVENT_ACTION_CANCEL:
                            break;
                        default:
                            LOGI("[%d] ????????? DEFAULT: [%d] %.2f %.2f", pointer_count,
                                 pointer_id, xpos, ypos);
                            return 0;
                    }
                    break;
            } // end switch
            // AINPUT_EVENT_TYPE_MOTION always handled
            return 1;
        case AINPUT_EVENT_TYPE_KEY:
            if (AKeyEvent_getKeyCode(event) == AKEYCODE_BACK) {
                if (engine->pLwc && engine->pLwc->puck_game) {
                    if (engine->pLwc->puck_game->game_state == LPGS_MAIN_MENU
                        && engine->pLwc->game_scene != LGS_LEADERBOARD) {
                        // just android handle this case --> destroy and stop this app "gracefully"
                    } else {
                        if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {
                            engine->pLwc->android_native_activity = engine->app;// engine->app->activity;
                            logic_emit_ui_event_async(engine->pLwc, "back_button", 0, 0);
                        }
                        return 1;
                    }
                }
            }
            // handle key input...
            break;
    } // end switch

    /*
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
     */

    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*) app->userData;
    switch (cmd) {
        case APP_CMD_START:
            LOGI("APP_CMD_START");
            break;
        case APP_CMD_RESUME:
            LOGI("APP_CMD_RESUME");
            engine->resumed = 1;
            ALooper_wake(engine->app->looper);
            logic_start_logic_update_job_async(engine->pLwc);
            break;
        case APP_CMD_PAUSE:
            LOGI("APP_CMD_PAUSE");
            engine->resumed = 0;
            logic_stop_logic_update_job_async(engine->pLwc);
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY");
            app->destroyRequested = 1;
            break;
        case APP_CMD_SAVE_STATE:
            LOGI("APP_CMD_SAVE_STATE");
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*) engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            if (engine && engine->pLwc && engine->pLwc->ttl) {
                lwttl_write_last_state(engine->pLwc->ttl, engine->pLwc);
            }
            break;
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW");
            // The window is being shown, get it ready.
            engine->app_cmd_init_window_triggered = true;
            engine->window_ready = 1;
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            // The window is being hidden or closed, clean it up.
            //eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            //glFlush();
            //engine_term_display(engine);
            engine->window_ready = 0;
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS");
            engine->focused = 1;
            ALooper_wake(engine->app->looper);
            recreate_surface(engine);
            /*
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L/60)*1000);
            }
             */
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS");
            engine->focused = 0;
            /*
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }

            if (engine->pLwc)
            {
                lw_deinit(engine->pLwc);
            }
            engine_draw_frame(engine);
             */
            break;
        case APP_CMD_WINDOW_RESIZED:
            // This callback will not be called because of Android bug.
            // https://issuetracker.google.com/issues/37054453
            // https://stackoverflow.com/questions/32587572/app-cmd-window-resized-is-not-called-but-native-window-is-resized
            LOGI("APP_CMD_WINDOW_RESIZED");
            break;
        case APP_CMD_CONFIG_CHANGED:
            LOGI("APP_CMD_CONFIG_CHANGED");
            recreate_surface(engine);
            break;
        case APP_CMD_LOW_MEMORY:
            LOGI("APP_CMD_LOW_MEMORY");
            break;
        default:
            LOGI("Unknown APP_CMD_ received: %d", cmd);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
static struct engine* shared_engine;

void android_main(struct android_app* state) {
    struct engine engine;
    shared_engine = &engine;

    LOGI("android_main");
    LOGI("internal data path: %s", state->activity->internalDataPath);

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();

    /*
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
                                        engine.sensorManager,
                                        ASENSOR_TYPE_ACCELEROMETER);
                                        */

    engine.sensorEventQueue = ASensorManager_createEventQueue(
            engine.sensorManager,
            state->looper, LOOPER_ID_USER,
            NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*) state->savedState;
    }

    // Setup static variables for calling Java functions from C++ side
    {
        JNIEnv* env;
        JavaVM* lJavaVM = state->activity->vm;

        s_vm_from_cpp = lJavaVM;
        s_obj_from_cpp = state->activity->clazz;

        s_looper_from_cpp = engine.app->looper;
    }

    struct timespec last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);
    double delta_time = 0.1;
    // loop waiting for stuff to do.
    long loop_tick = 0;

    while (1) {
        struct timespec loop_begin;
        clock_gettime(CLOCK_MONOTONIC, &loop_begin);

        // initialization of engine display and lw
        if (engine.inited == false
            && engine.app_cmd_init_window_triggered
            && s_java_activity_created
            && engine.app->window != NULL
            && engine.resumed
            && engine.focused) {
            LOGI("Init engine...");
            engine_init_display(&engine);
            if (engine.pLwc) {
                lw_deinit(engine.pLwc);
            }
            engine.pLwc = lw_init_initial_size(engine.width, engine.height);
            engine.pLwc->internal_data_path = state->activity->internalDataPath;
            engine.pLwc->user_data_path = state->activity->internalDataPath;
			lw_set_viewport_size(engine.pLwc, engine.width, engine.height);
            lw_set_window_size(engine.pLwc, engine.width, engine.height);
            engine.inited = true;
            lwc_start_logic_thread(engine.pLwc);
        }
        // update width & height on LWCONTEXT side if inconsistency with engine found
        if (engine.pLwc) {
            if (engine.width != engine.pLwc->window_width || engine.height != engine.pLwc->window_height) {
				lw_set_viewport_size(engine.pLwc, engine.width, engine.height);
                lw_set_window_size(engine.pLwc, engine.width, engine.height);
            }
        }
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        int poll_without_timeout = engine.focused && engine.resumed;

        /*
        LOGI("Loop Tick: %d, 1=%d, 2=%d, 3=%d",
             loop_tick++,
             engine.inited,
             engine.focused,
             engine.resumed);
        */

        while ((ident = ALooper_pollAll(poll_without_timeout ? 0 : -1, NULL, &events,
                                        (void**) &source)) >= 0) {
            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            /*
            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {


                        LOGI("accelerometer: x=%f y=%f z=%f",
                             event.acceleration.x, event.acceleration.y,
                             event.acceleration.z);

                    }
                }
            }
             */

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        // Done with events; draw next animation frame.
        engine.state.angle += .01f;
        if (engine.state.angle > 1) {
            engine.state.angle = 0;
        }

        if (engine.pLwc) {
            //lwc_update(engine.pLwc, 1.0/60);

            const int uc = lw_get_update_count(engine.pLwc);
            const int rc = lw_get_render_count(engine.pLwc);
            const long lt_sec = deltatime_last_time_sec(engine.pLwc->update_dt);
            const long lt_nsec = deltatime_last_time_nsec(engine.pLwc->update_dt);
            const double dt = lwcontext_delta_time(engine.pLwc);
            const double throttle_time = 1 / 60.0 - dt;

            //LOGI("U# %6d / R# %6d / Last time sec: %ld, nsec: %ld / Delta time: %.4f / FPS: %.4f fps", uc, rc, lt_sec, lt_nsec, dt, 1.0/dt);

        }


        // Drawing is throttled to the screen update rate, so there
        // is no need to do timing here.
        engine_draw_frame(&engine);

        struct timespec loop_end;
        clock_gettime(CLOCK_MONOTONIC, &loop_end);


        /*
        {
            long nsec_diff = loop_end.tv_nsec - loop_begin.tv_nsec;
            long sec_diff = loop_end.tv_sec - loop_begin.tv_sec;

            if (nsec_diff < 0)
            {
                nsec_diff += 1000000000LL;
                sec_diff--;
            }

            long frame_spent_nanoseconds = sec_diff * 1000000000LL + nsec_diff;
            long target_nanoseconds = 16666667; // 1/60 sec (rounded)
            long throttle_time = target_nanoseconds - frame_spent_nanoseconds;
            // manual throttling
            if (throttle_time > 0)
            {
                //LOGI("throttle_time: %ld nanosecs", throttle_time);

                usleep(throttle_time / 1000);
            }
        }
        */

        {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            long nsec_diff = current_time.tv_nsec - last_time.tv_nsec;
            long sec_diff = current_time.tv_sec - last_time.tv_sec;

            if (nsec_diff < 0) {
                nsec_diff += 1000000000LL;
                sec_diff--;
            }

            uint64_t frame_interval_nanoseconds = sec_diff * 1000000000ULL + nsec_diff;
            delta_time = (double) frame_interval_nanoseconds / 1e9;
            //LOGI("frame interval: %ld nsec / %.4f fps", frame_interval_nanoseconds, 1.0 / delta_time);

            last_time = current_time;
        }

    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_popsongremix_laidoff_LaidoffNativeActivity_getUserId(JNIEnv* env, jobject obj) {
    s_env_from_java = env;
    s_obj_from_java = obj;
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_popsongremix_laidoff_LaidoffNativeActivity_signalResourceReady(JNIEnv* env, jobject obj,
                                                                        jclass and9NativeActivityClass,
                                                                        int downloadAssets) {
    s_env_from_java = env;
    s_obj_from_java = obj;

    env->GetJavaVM(&s_vm_from_java);

    std::string hello = "Hello from C++";

    s_java_activity_created = true;
    s_download_assets = downloadAssets;

    ALooper_wake(s_looper_from_cpp);

    return env->NewStringUTF(hello.c_str());
}

extern "C" int lw_download_assets() {
    return s_download_assets;
}

extern "C" void lw_app_quit(LWCONTEXT* pLwc, void* native_context) {
    // 'native_context' is 'android_app'
    struct android_app* android_app = (struct android_app*) native_context;
    ANativeActivity_finish(android_app->activity);
    android_app->destroyRequested = 1;
}

extern "C" void lw_start_text_input_activity(LWCONTEXT* pLwc, int tag) {
    pLwc->last_text_input_seq = lw_get_text_input_seq();
    lw_set_text_input_tag(tag);
    request_void_string_command("startTextInputActivity", "dummy");
}

extern "C" void lw_start_reward_video(LWCONTEXT* pLwc, int tag) {
    request_void_string_command("startRewardVideo", "dummy");
}

extern "C" void lw_start_sign_in(LWCONTEXT* pLwc, int tag) {
    int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    if (pLwc->tcp) {
        v1 = pLwc->tcp->user_id.v[0];
        v2 = pLwc->tcp->user_id.v[1];
        v3 = pLwc->tcp->user_id.v[2];
        v4 = pLwc->tcp->user_id.v[3];
    }
    request_void_int4_command("startSignIn", v1, v2, v3, v4);
}

extern "C" void lw_start_sign_out(LWCONTEXT* pLwc, int tag) {
    request_void_string_command("startSignOut", "dummy");
}

static char push_token[1024];

extern "C" void lw_request_remote_notification_device_token(LWCONTEXT* pLwc) {
    request_void_long_command("requestPushToken", (jlong) pLwc);

    //lw_set_push_token(pLwc, 2, push_token);
}

extern "C" JNIEXPORT void JNICALL
Java_com_popsongremix_laidoff_LaidoffFirebaseInstanceIDService_setPushToken(JNIEnv* env, jclass cls,
                                                                            jstring text) {
    const char* buffer = env->GetStringUTFChars(text, JNI_FALSE);

    size_t buffer_len = strlen(buffer);
    if (buffer_len > 0) {
        strcpy(push_token, buffer);
        push_token[strlen(buffer)] = '\0';
    } else {
        push_token[0] = '\0';
    }

    env->ReleaseStringUTFChars(text, buffer);
}

extern "C" JNIEXPORT void JNICALL
Java_com_popsongremix_laidoff_LaidoffNativeActivity_setPushTokenAndSend(JNIEnv* env, jclass cls,
                                                                        jstring text,
                                                                        jlong pLwcLong) {
    Java_com_popsongremix_laidoff_LaidoffFirebaseInstanceIDService_setPushToken(env, cls, text);
    lw_set_push_token((LWCONTEXT*) pLwcLong, 2, push_token);
}

extern "C" JNIEXPORT void JNICALL
Java_com_popsongremix_laidoff_LaidoffNativeActivity_setWindowSize(JNIEnv* env, jclass cls, jint w,
                                                                  jint h, jlong pLwcLong) {
    if (shared_engine) {
        // no size change - do nothing
        if (shared_engine->width == w && shared_engine->height == h) {
            return;
        }
        // width-height switched (might in sleep mode) - do nothing
        if (shared_engine->width == h && shared_engine->height == w) {
            return;
        }
        shared_engine->width = w;
        shared_engine->height = h;
        LWCONTEXT* pLwc = pLwcLong ? (LWCONTEXT*) pLwcLong : shared_engine->pLwc;
        if (pLwc) {
			lw_set_viewport_size(pLwc, w, h);
            lw_set_window_size(pLwc, w, h);
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_popsongremix_laidoff_LaidoffNativeActivity_lw_on_destroy(JNIEnv* env, jclass cls, jlong pLwcLong) {
    if (shared_engine) {
        LWCONTEXT* pLwc = pLwcLong ? (LWCONTEXT*) pLwcLong : shared_engine->pLwc;
        if (pLwc) {
            lw_on_destroy(pLwc);
        }
    }
}
