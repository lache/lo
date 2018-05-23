#pragma once

typedef struct GLFWwindow GLFWwindow;

#ifdef __cplusplus
extern "C" {;
#endif
void lwimgui_init(GLFWwindow* window);
void lwimgui_render(GLFWwindow* window);
void lwimgui_shutdown();
void lwimgui_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void lwimgui_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void lwimgui_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void lwimgui_char_callback(GLFWwindow* window, unsigned int c);
int lwimgui_want_capture_mouse();
int lwimgui_want_capture_keyboard();
int lwimgui_want_text_input();
#ifdef __cplusplus
}
#endif
