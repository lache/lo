#include "lwimgui.h"
#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
//#include <GLFW/glfw3.h>
#include "lwcontext.h"
#include "lwttl.h"
#include "lwlog.h"
#include <stdio.h>
#include "iconchar.h"
#include <ctype.h> // isspace() on mac os
static bool show_test_window = false;
static bool show_chat_window = false;
static bool show_another_window = true;
static ImVec4 clear_color = ImColor(114, 144, 154);

extern "C" void lwimgui_init(GLFWwindow* window) {
	// Callback for ImGui will be installed manually.
	ImGui_ImplGlfwGL3_Init(window, false);
    ImGuiIO& io = ImGui::GetIO();
    //ImFont* font0 = io.Fonts->AddFontDefault();

    ImFontConfig font_config;
    memset(&font_config, 0, sizeof(ImFontConfig));
    font_config.OversampleH = 2; //or 2 is the same
    font_config.OversampleV = 2;
    font_config.PixelSnapH = 1;

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // ASCII
        0x1100, 0x11FF, // Korean (Jamo)
        0x2032, 0x2033, // Minutes, Seconds (for DMS representation)
        0x3130, 0x318F, // Korean (Jamo; compatability)
        0xAC00, 0xD7AF, // Korean
        0,
    };

#if LW_PLATFORM_WIN32
    // mac os resource finding problem not solved
    ImFont* font1 = io.Fonts->AddFontFromFileTTF("fonts" PATH_SEPARATOR "NotoSansCJKkr-Regular.otf",
                                                 18,
                                                 nullptr,
                                                 ranges);
#endif
    
    //ImFontConfig config;
    //config.OversampleH = 3;
    //config.OversampleV = 1;
    //config.GlyphOffset.y -= 2.0f;      // Move everything by 2 pixels up
    //config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
    //io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, &config);
}

static char* trimwhitespace_inplace(char* str) {
    char* end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

extern "C" void lwimgui_render(GLFWwindow* window) {
    LWCONTEXT* pLwc = (LWCONTEXT*)glfwGetWindowUserPointer(window);
    ImGui_ImplGlfwGL3_NewFrame();
    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    //{
    //	static float f = 0.0f;
    //	ImGui::Text("Hello, world!");
    //	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    //	ImGui::ColorEdit3("clear color", (float*)&clear_color);
    //	if (ImGui::Button("Test Window")) show_test_window ^= 1;
    //	if (ImGui::Button("Another Window")) show_another_window ^= 1;
    //	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    //}
    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window) {
        if (show_chat_window) {
            static char buf[256] = "";
            static bool focus_here = false;
            const int chat_window_height = 250;
            ImGui::SetNextWindowPos(ImVec2(0, (float)pLwc->window_height - chat_window_height));
            if (!ImGui::Begin("Chat",
                              &show_chat_window,
                              ImVec2((float)pLwc->window_width, (float)chat_window_height),
                              0.8f,
                              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::End();
                return;
            }
            ImGui::Text("%s", u8"채팅"); ImGui::SameLine();
            if (ImGui::SmallButton(u8"모두 삭제")) {
                lwchatringbuffer_clear(&pLwc->chat_ring_buffer);
            }
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
            int chat_lines = lwchatringbuffer_count(&pLwc->chat_ring_buffer);
            for (int i = 0; i < chat_lines; i++) {
                const LWCHATLINE* cl = lwchatringbuffer_get(&pLwc->chat_ring_buffer, i);
                ImGui::TextUnformatted(cl->line);
                ImGui::SameLine(ImGui::GetWindowWidth() - 130);
                //ImGui::TextUnformatted(ctime(&cl->rawtime));
                ImGui::TextUnformatted(cl->strtime);
            }
            if (lwchatringbuffer_flush_scroll_to_bottom(&pLwc->chat_ring_buffer)) {
                ImGui::SetScrollHere();
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();
            ImGui::Separator();
            //ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
            //ImGui::Text("%s Hello...admin", u8"으흐흐흐");
            if (focus_here) {
                ImGui::SetKeyboardFocusHere();
                focus_here = false;
            }
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("Chat", buf, ARRAY_SIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                const char* buf_trimmed = trimwhitespace_inplace(buf);
                if (buf_trimmed[0]) {
                    LOGI("Chat %s", buf_trimmed);
                    lwttl_udp_send_ttlchat(pLwc->ttl, lwttl_sea_udp(pLwc->ttl), buf_trimmed);
                    //lwchatringbuffer_add(&pLwc->chat_ring_buffer, buf_trimmed);
                    //scroll_to_bottom = true;
                }
                buf[0] = 0;
                focus_here = true;
            }
            ImGui::End();
        }
    
        ImGui::SetNextWindowPos(ImVec2(100, 0), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 60), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Admin", &show_another_window);
        {
            LWTTLLNGLAT selected_lnglat;
            if (lwttl_selected(pLwc->ttl, &selected_lnglat)) {
                ImGui::Text("Selected Cell LNG %f, LAT %f", selected_lnglat.lng, selected_lnglat.lat);
                int lng_d, lng_m; float lng_s;
                int lat_d, lat_m; float lat_s;
                lwttl_degrees_to_dms(&lng_d, &lng_m, &lng_s, selected_lnglat.lng);
                lwttl_degrees_to_dms(&lat_d, &lat_m, &lat_s, selected_lnglat.lat);
                ImGui::Text("LNG %d%s%d%s%f%s, LAT %d%s%d%s%f%s",
                            lng_d, LW_UTF8_DEGREES, lng_m, LW_UTF8_MINUTES, lng_s, LW_UTF8_SECONDS,
                            lat_d, LW_UTF8_DEGREES, lat_m, LW_UTF8_MINUTES, lat_s, LW_UTF8_SECONDS);
                int xc0, yc0;
                lwttl_selected_int(pLwc->ttl, &xc0, &yc0);
                const LWPTTLSINGLECELL* p = lwttl_single_cell(pLwc->ttl);
                ImGui::Text("XC %d YC %d [attr 0x%08X]", xc0, yc0, p->attr);
            } else {
                ImGui::Text("Not selected");
            }
            vec3 cam_eye;
            lwttl_cam_eye(pLwc->ttl, cam_eye);
            bool eye_changed = false;
            eye_changed |= ImGui::SliderFloat("eye x", &cam_eye[0], -20.0f, +20.0f);
            eye_changed |= ImGui::SliderFloat("eye y", &cam_eye[1], -20.0f, +20.0f);
            eye_changed |= ImGui::SliderFloat("eye z", &cam_eye[2], -20.0f, +20.0f);
            if (eye_changed) {
                lwttl_set_cam_eye(pLwc->ttl, cam_eye);
            }
            vec3 cam_look_at;
            lwttl_cam_look_at(pLwc->ttl, cam_look_at);
            bool look_at_changed = false;
            look_at_changed |= ImGui::SliderFloat("look at x", &cam_look_at[0], -20.0f, +20.0f);
            look_at_changed |= ImGui::SliderFloat("look at y", &cam_look_at[1], -20.0f, +20.0f);
            look_at_changed |= ImGui::SliderFloat("look at z", &cam_look_at[2], -20.0f, +20.0f);
            if (look_at_changed) {
                lwttl_set_cam_look_at(pLwc->ttl, cam_look_at);
            }
            if (eye_changed || look_at_changed) {
                lwttl_update_aspect_ratio(pLwc->ttl, pLwc->viewport_width, pLwc->viewport_height);
            }
        }
        if (ImGui::Button("Test Window")) {
            show_test_window ^= 1;
        } ImGui::SameLine();
        if (ImGui::Button("Chat Window")) {
            show_chat_window ^= 1;
        }
        ImGui::End();
    }
    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window) {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }
    ImGui::Render();
}

extern "C" void lwimgui_shutdown() {
    ImGui_ImplGlfwGL3_Shutdown();
}

extern "C" void lwimgui_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
}

extern "C" void lwimgui_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}

extern "C" void lwimgui_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}

extern "C" void lwimgui_char_callback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfwGL3_CharCallback(window, c);
}

extern "C" int lwimgui_want_capture_mouse() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

extern "C" int lwimgui_want_capture_keyboard() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

extern "C" int lwimgui_want_text_input() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}
