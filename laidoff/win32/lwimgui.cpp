#include "lwimgui.h"
#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
//#include <GLFW/glfw3.h>
#include "lwcontext.h"
#include "lwttl.h"
#include "lwlog.h"

static bool show_test_window = false;
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
        0xAC00, 0xD7AF, // Korean
        0,
    };

    ImFont* font1 = io.Fonts->AddFontFromFileTTF("C:\\Users\\gasbank\\Downloads\\NotoSansCJKkr-hinted\\NanumGothicCoding.ttf",
                                                 16,
                                                 nullptr,
                                                 ranges);
    //ImFontConfig config;
    //config.OversampleH = 3;
    //config.OversampleV = 1;
    //config.GlyphOffset.y -= 2.0f;      // Move everything by 2 pixels up
    //config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
    //io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, &config);
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
	if (show_another_window)
	{
        bool open = false;
        const int chat_window_height = 150;
        ImGui::SetNextWindowPos(ImVec2(0, (float)pLwc->window_height - chat_window_height));
        if (!ImGui::Begin("Example: Fixed Overlay",
                          &open,
                          ImVec2((float)pLwc->window_width, (float)chat_window_height),
                          1.0f,
                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::End();
            return;
        }
        ImGui::Text("%s", u8"누가 봐도 채팅창");
        ImGui::Separator();
        //ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        //ImGui::Text("%s Hello...admin", u8"으흐흐흐");
        static char buf[256] = u8"한글을 입력 해 보세요. 안될테니까요.";
        if (ImGui::InputText("Chat", buf, ARRAY_SIZE(buf))) {
            LOGI("Chat %s", buf);
            lwttl_udp_send_ttlchat(pLwc->ttl, lwttl_sea_udp(pLwc->ttl), buf);
        }
        ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(100, 0), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(500, 60), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Admin", &show_another_window);
        {
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
                lwttl_update_view_proj(pLwc->ttl, pLwc->viewport_aspect_ratio);
            }
        }
		if (ImGui::Button("Test Window")) {
			show_test_window ^= 1;
		} ImGui::SameLine();
		if (ImGui::Button("Test Window 2")) {
			show_test_window ^= 1;
		}
		ImGui::End();
	}
	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
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