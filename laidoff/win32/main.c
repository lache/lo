#include <time.h>
#include "lwgl.h"
#include "laidoff.h"
#include "czmq.h"
//#include "lwdeltatime.h"
#include "lwtimepoint.h"
#include "lwlog.h"
#include "lwudp.h"
//#include "script.h"
#include "lwime.h"
#if LW_PLATFORM_WIN32
#include "scriptwatch.h"
#include "lwimgui.h"
#include "resource.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif
#include "rmsg.h"

#ifndef BOOL
#define BOOL int
#endif

// Working directory change
#if LW_PLATFORM_OSX || LW_PLATFORM_RPI
#include <dirent.h>
#include <unistd.h>
#endif

#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

#define INITIAL_SCREEN_RESOLUTION_Y (640*2) //(1920)
#define INITIAL_SCREEN_RESOLUTION_X (360*2) //(1080)

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_pos_callback(GLFWwindow* window, double x, double y);
void char_callback(GLFWwindow* window, unsigned int c);
void destroy_ext_sound_lib();

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static BOOL directory_exists(const char* szPath) {
#if LW_PLATFORM_WIN32
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    DIR* dir = opendir(szPath);
    if (dir) {
        return 1;
    } else {
        return 0;
    }
#endif
}

void window_size_callback(GLFWwindow* window, int width, int height) {
    LWCONTEXT* pLwc = (LWCONTEXT*)glfwGetWindowUserPointer(window);

    lw_set_window_size(pLwc, width, height);
    lw_set_viewport_size(pLwc, width, height);
}

static GLFWwindow* create_glfw_window() {
    GLFWwindow* window = glfwCreateWindow(INITIAL_SCREEN_RESOLUTION_X,
                                          INITIAL_SCREEN_RESOLUTION_Y,
                                          "LAID OFF",
                                          NULL,
                                          NULL);
#if LW_PLATFORM_WIN32
    HWND hwnd = glfwGetWin32Window(window);
    int scaling_factor = GetDpiForWindow(hwnd) / 96;
    glfwSetWindowSize(window,
                      INITIAL_SCREEN_RESOLUTION_X * scaling_factor,
                      INITIAL_SCREEN_RESOLUTION_Y * scaling_factor);
#endif
    return window;
}

#if LW_PLATFORM_WIN32
static int make_multiple_instances_nonoverlapping(GLFWwindow* window, const RECT* work_area, int window_rect_to_client_rect_dx, int window_rect_to_client_rect_dy, int width, int height) {
    HANDLE first_mutex = CreateMutex(NULL, TRUE, "laidoff-win32-client-mutex-1");
    int first_window_exists = GetLastError() == ERROR_ALREADY_EXISTS;
    if (first_window_exists) {
        CloseHandle(first_mutex);
        HANDLE second_mutex = CreateMutex(NULL, TRUE, "laidoff-win32-client-mutex-2");
        int second_window_exists = GetLastError() == ERROR_ALREADY_EXISTS;
        if (second_window_exists) {
            CloseHandle(second_mutex);
            HANDLE third_mutex = CreateMutex(NULL, TRUE, "laidoff-win32-client-mutex-3");
            int third_window_exists = GetLastError() == ERROR_ALREADY_EXISTS;
            if (third_window_exists) {
                CloseHandle(third_mutex);
                // fourth position
                glfwSetWindowPos(window, work_area->left + window_rect_to_client_rect_dx + width + window_rect_to_client_rect_dx, work_area->top + window_rect_to_client_rect_dy + height + window_rect_to_client_rect_dy);
                return 3;
            } else {
                // third position
                glfwSetWindowPos(window, work_area->left + window_rect_to_client_rect_dx, work_area->top + window_rect_to_client_rect_dy + height + window_rect_to_client_rect_dy);
                return 2;
            }
        } else {
            // second position
            glfwSetWindowPos(window, work_area->left + window_rect_to_client_rect_dx + width + window_rect_to_client_rect_dx, work_area->top + window_rect_to_client_rect_dy);
            return 1;
        }
    }
    return 0;
}
#endif

#if LW_PLATFORM_OSX
void test_main_bundle_path(const char* filename);
#endif

int main(int argc, char* argv[]) {
    LOGI("LAIDOFF: Greetings.");

    while (!directory_exists("assets") && LwChangeDirectory("..")) {
    }

#if LW_PLATFORM_OSX
    char cwd[2048];
    getcwd(cwd, 2048);
    LOGI("CWD: %s", cwd);
    test_main_bundle_path("test.resource");
#endif

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    /*
    OpenGL Version -	GLSL Version
    2.0 -	110
    2.1 -	120
    3.0 -	130
    3.1 -	140
    3.2 -	150
    3.3 -	330
    4.0 -	400
    4.1 -	410
    4.2 -	420
    4.3 -	430
    */

#if (!LW_PLATFORM_RPI && !LW_PLATFORM_LINUX)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#endif

    glfwWindowHint(GLFW_DEPTH_BITS, 16);

    // enable supersampling (fullscreen antialiasing)
    glfwWindowHint(GLFW_SAMPLES, 8);

#if LW_PLATFORM_RPI
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif

    // Borderless or bordered window
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = create_glfw_window();

    if (!window) {
        // Try with OpenGL API again
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

        window = create_glfw_window();

        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    }
#if LW_PLATFORM_WIN32
    HWND hwnd = glfwGetWin32Window(window);
    int scaling_factor = GetDpiForWindow(hwnd) / 96;
    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    RECT window_rect;
    GetWindowRect(hwnd, &window_rect);
    int window_rect_width = window_rect.right - window_rect.left;
    int window_rect_height = window_rect.bottom - window_rect.top;
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);
    int client_rect_width = client_rect.right - client_rect.left;
    int client_rect_height = client_rect.bottom - client_rect.top;
    int window_rect_to_client_rect_dx = window_rect_width - client_rect_width;
    int window_rect_to_client_rect_dy = window_rect_height - client_rect_height;

    glfwSetWindowPos(window, work_area.left + window_rect_to_client_rect_dx, work_area.top + window_rect_to_client_rect_dy);
#elif LW_PLATFORM_OSX
    glfwSetWindowPos(window, 0, 0);
#endif
    // Register glfw event callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCharCallback(window, char_callback);
    // Make OpenGL context current
    glfwMakeContextCurrent(window);
#if !LW_PLATFORM_RPI
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
    glfwSwapInterval(1);

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    srand((unsigned int)time(0));

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
#if LW_PLATFORM_WIN32
    int client_instance_id = make_multiple_instances_nonoverlapping(window, &work_area, window_rect_to_client_rect_dx, window_rect_to_client_rect_dy, width, height);
#else
    int client_instance_id = 0;
    if (argc >= 2) {
        client_instance_id = atoi(argv[1]);
}
#endif
    LWCONTEXT* pLwc = lw_init_initial_size(width, height);
    pLwc->argc = argc;
    pLwc->argv = (char**)argv;
    // multiple clients with different user data path enable
    // multiplayer testing environment on a single desktop.
    switch (client_instance_id) {
    case 0:
        break;
    case 1:
        pLwc->user_data_path = "user1";
        break;
    case 2:
        pLwc->user_data_path = "user2";
        break;
    case 3:
        pLwc->user_data_path = "user3";
        break;
    }


    lw_set_window(pLwc, window);

    lw_set_window_size(pLwc, width, height);
    lw_set_viewport_size(pLwc, width, height);

    glfwSetWindowUserPointer(window, pLwc);

    lwc_start_logic_thread(pLwc);

#if LW_PLATFORM_WIN32
    lwc_start_scriptwatch_thread(pLwc);
    lwimgui_init(window);
#endif

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        lwc_prerender_mutable_context(pLwc);
        lwc_render(pLwc);
#if LW_PLATFORM_WIN32
        lwimgui_render(window);
#endif
        glfwSwapBuffers(window);

    }
    // If glfw loop is terminated without a proper exit procedure ('rmsg_quitapp()')
    // (i.e., by clicking 'X' button on the window)
    // since glfw loop is finished, we just need to close logic loop.
    if (!pLwc->quit_request) {
        lw_flag_logic_actor_to_quit_and_wait(pLwc);
    }
#if LW_PLATFORM_WIN32
    lwimgui_shutdown();
#endif

    glfwDestroyWindow(window);

    glfwTerminate();

    destroy_ext_sound_lib();

    lw_on_destroy(pLwc);

    LOGI("LAIDOFF: Goodbye.");

    exit(EXIT_SUCCESS);
}

void lw_app_quit(LWCONTEXT* pLwc, void* native_context) {
    glfwSetWindowShouldClose(lw_get_window(pLwc), GLFW_TRUE);
}

#if LW_PLATFORM_WIN32
INT_PTR CALLBACK TextInputProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    wchar_t lpszTextInput[128];
    WORD cchTextInput;

    switch (message) {
    case WM_INITDIALOG:
        // Set the default push button to "Cancel." 
        SendDlgItemMessageW(hDlg, IDC_EDIT1, WM_SETFOCUS, 0, 0);
        /*SendMessage(hDlg,
                    DM_SETDEFID,
                    (WPARAM)IDCANCEL,
                    (LPARAM)0);*/

        return TRUE;

    case WM_COMMAND:
        // Set the default push button to "OK" when the user enters text. 
        if (HIWORD(wParam) == EN_CHANGE &&
            LOWORD(wParam) == IDC_EDIT1) {
            SendMessage(hDlg,
                        DM_SETDEFID,
                        (WPARAM)IDOK,
                        (LPARAM)0);
        }
        switch (wParam) {
        case IDOK:
            // Get number of characters. 
            cchTextInput = (WORD)SendDlgItemMessageW(hDlg,
                                                     IDC_EDIT1,
                                                     EM_LINELENGTH,
                                                     (WPARAM)0,
                                                     (LPARAM)0);
            if (cchTextInput >= ARRAY_SIZE(lpszTextInput) / 2) {
                MessageBoxW(hDlg,
                            L"Too many characters.",
                            L"Error",
                            MB_OK);

                EndDialog(hDlg, TRUE);
                return FALSE;
            } else if (cchTextInput == 0) {
                MessageBoxW(hDlg,
                            L"No characters entered.",
                            L"Error",
                            MB_OK);

                EndDialog(hDlg, TRUE);
                return FALSE;
            }

            // Put the number of characters into first word of buffer. 
            *((LPWORD)lpszTextInput) = cchTextInput;

            // Get the characters. 
            SendDlgItemMessageW(hDlg,
                                IDC_EDIT1,
                                EM_GETLINE,
                                (WPARAM)0,       // line 0 
                                (LPARAM)lpszTextInput);

            // Null-terminate the string. 
            lpszTextInput[cchTextInput] = 0;

            //MessageBoxW(hDlg,
            //		   lpszTextInput,
            //		   L"Did it work?",
            //		   MB_OK);

            // Call a local password-parsing function. 
            //ParsePassword(lpszTextInput);
            WideCharToMultiByte(CP_UTF8,
                                0,
                                lpszTextInput,
                                cchTextInput,
                                lw_get_text_input_for_writing(),
                                lw_text_input_max(),
                                0,
                                0);
            lw_increase_text_input_seq();

            EndDialog(hDlg, TRUE);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        return 0;
        //default:
        //    return DefWindowProc(hDlg, message, wParam, lParam);
        //    break;
    }
    return FALSE;
}

void lw_start_text_input_activity(LWCONTEXT* pLwc, int tag) {
    HWND hWnd = glfwGetWin32Window(pLwc->window);
    HINSTANCE hInst = GetModuleHandle(NULL);
    lw_set_text_input_tag(tag);
    DialogBox(hInst,							// application instance
              MAKEINTRESOURCE(IDD_DIALOG2),		// dialog box resource
              hWnd,								// owner window
              TextInputProc);					// dialog box window procedure
}
#else
void lw_start_text_input_activity(LWCONTEXT* pLwc, int tag) {
    LOGE("Not implemented yet..");
}
#endif

void lw_request_remote_notification_device_token(LWCONTEXT* pLwc) {
    LOGE("Not supported in win32");
}

void lw_start_reward_video(LWCONTEXT* pLwc, int tag) {
    LOGE("Not supported");
}

void lw_start_sign_in(LWCONTEXT* pLwc, int tag) {
    LOGE("Not supported");
}

void lw_start_sign_out(LWCONTEXT* pLwc, int tag) {
    LOGE("Not supported");
}
