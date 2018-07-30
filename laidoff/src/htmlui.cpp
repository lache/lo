#include <stdio.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <locale>
#include "text_container.h"
#include "render_command_container.h"
#include "htmlui.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "file.h"
#include "render_ttl.h"
#include "lwtcp.h"
#include "lwmutex.h"
#include "logic.h"
#include "lwlog.h"
#include "lwtimepoint.h"
#include "script.h"

class LWHTMLUI {
public:
    LWHTMLUI(LWCONTEXT* _pLwc, int w, int h)
        : pLwc(_pLwc)
        , container(_pLwc, w, h)
        , client_width(w)
        , client_height(h)
        , refresh_html_body(0)
        , touch_rect(4)
        , scroll_y(0)
        , scroll_dy(0)
        , mouse_down(false)
        , mouse_down_nx(0)
        , mouse_down_ny(0)
        , mouse_dragging(false) {
        LWMUTEX_INIT(parsing_mutex);
        std::shared_ptr<char> master_css_str(create_string_from_file(ASSETS_BASE_PATH "css" PATH_SEPARATOR "master.css"), free);
        browser_context.load_master_stylesheet(master_css_str.get());

        browser_context.set_loop_var_map(
            {
                { "button-loop" ,
            {
                {
                    "name", { "button1", "button2" }
                },{
                    "desc", { "button1-desc", "button2-desc" }
                },{
                    "script",{ "script:call_me(1)", "script:call_me(2)" }
                }, } }, });

        htmlui_update_country_data(pLwc, this);
    }
    ~LWHTMLUI() {
        LWMUTEX_DESTROY(parsing_mutex);
    }
    void set_loop_var_map_entry(const std::string& loop_name, const std::map<std::string, std::vector<std::string> >& var_map) {
        browser_context.set_loop_var_map_entry(loop_name, var_map);
    }
    void set_loop_key_value(const std::string& loop_name, const std::string& key, const std::string& value) {
        lock();
        browser_context.set_loop_key_value(loop_name, key, value);
        unlock();
    }
    void clear_loop(const std::string& loop_name) {
        lock();
        browser_context.clear_loop(loop_name);
        unlock();
    }
    bool load_page(const char* html_path) {
        std::shared_ptr<char> html_str(create_string_from_file(html_path), free);
        if (html_str) {
            load_body(html_str.get());
            return true;
        }
        return false;
    }
    void load_body(const char* html_body) {
        lock();
        load_body_internal(html_body);
        last_html_str = html_body;
        unlock();
    }
    void load_body_internal(const char* html_body) {
        container.clear_remtex_name_hash_set();
        container.clear_render_command_queue();
        scroll_y = 0;
        scroll_dy = 0;
        doc = litehtml::document::createFromString(html_body, &container, &browser_context);
        // root document size can be obtained after calling doc->render()
    }
    void lock() {
        LWMUTEX_LOCK(parsing_mutex);
    }
    void unlock() {
        LWMUTEX_UNLOCK(parsing_mutex);
    }
    void render_page() {
        doc->render(client_width);
        document_size.width = 0;
        document_size.height = 0;
        doc->root()->calc_document_size(document_size);
        LOGI("HTML UI document size: %d x %d", document_size.width, document_size.height);
    }
    void draw() {
        litehtml::position clip(0, 0, document_size.width, document_size.height);
        doc->draw(0, 0, 0, &clip);
    }
    void redraw_fbo() {
        LOGI("Redrawing FBO with the same HTML data with client size %d x %d...", client_width, client_height);
        lock();
        load_body_internal(last_html_str.c_str());
        render_page();
        if (lwfbo_prerender(pLwc, &pLwc->shared_fbo) == 0) {
            draw();
            lwfbo_postrender(pLwc, &pLwc->shared_fbo);
        }
        unlock();
    }
    void on_lbutton_down(float nx, float ny) {
        if (doc) {
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height + scroll_y + scroll_dy));
            litehtml::position::vector redraw_boxes;
            doc->on_mouse_over(x, y, x, y, redraw_boxes);
            doc->on_lbutton_down(x, y, x, y, redraw_boxes);
            last_lbutton_down_element = doc->root()->get_element_by_point(x, y, x, y);
        }
        if (over_element(nx, ny)) {
            mouse_down = true;
            mouse_down_nx = nx;
            mouse_down_ny = ny;
        }
    }
    void on_lbutton_up(float nx, float ny) {
        if (doc) {
            litehtml::position::vector redraw_boxes;
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height + scroll_y + scroll_dy));
            if (last_lbutton_down_element == doc->root()->get_element_by_point(x, y, x, y)) {
                doc->on_mouse_over(x, y, x, y, redraw_boxes);
                if (mouse_dragging == false) {
                    doc->on_lbutton_up(x, y, x, y, redraw_boxes);
                }
            }
            last_lbutton_down_element.reset();
        }
        mouse_down = false;
        mouse_dragging = false;
        scroll_y += scroll_dy;
        scroll_dy = 0;
    }
    void on_over(float nx, float ny) {
        if (doc) {
            // too slow to execute in render frame basis...
            // litehtml::position::vector redraw_boxes;
            // int x = static_cast<int>(roundf(nx * client_width));
            // int y = static_cast<int>(roundf(ny * client_height)) + scroll_y;
            // doc->on_mouse_over(x, y, x, y, redraw_boxes);
        }
        if (mouse_down) {
            float dny = ny - mouse_down_ny;
            if (mouse_dragging || fabsf(dny) > 0.01f) {
                mouse_dragging = true;
                scroll_dy = -dny * client_height;
            }
        }
    }
    void set_next_html_path(const char* html_path) {
        next_html_path = html_path;
    }
    void set_refresh_html_body(int v) {
        if (refresh_html_body == 0 && v == 1) {
            const char* s = "on_set_refresh_html_body()";
            logic_emit_evalute_with_name_async(pLwc, s, strlen(s), s);
        }
        refresh_html_body = v;
    }
    int get_refresh_html_body() const {
        return refresh_html_body;
    }
    void load_next_html_path() {
        if (next_html_path.empty() == false) {
            lwc_render_ttl_fbo(pLwc, next_html_path.c_str());
            next_html_path.clear();
        }
    }
    void load_next_html_body() {
        if (refresh_html_body) {
			LOGIx("=== load_next_html_body ===");
			LOGIx(pLwc->tcp_ttl->html_body);
			if (pLwc->tcp_ttl) {
                if (pLwc->game_scene == LGS_TTL) {
                    lwc_render_ttl_fbo_body(pLwc, pLwc->tcp_ttl->html_body);
                } else if (pLwc->game_scene == LGS_GAZZA) {
                    // online Gazza!!!!
                    char s[1024*512];
                    sprintf(s, "on_json_body([===[%s]===])", pLwc->tcp_ttl->html_body);
                    size_t s_len = strlen(s);
                    /*for (size_t i = 0; i < s_len; i++) {
                        if (s[i] == '\\') {
                            s[i] = '_';
                        }
                    }*/
                    logic_emit_evalute_with_name_async(pLwc, s, s_len, s);
                }
			} else {
				// offline
				lwc_render_ttl_fbo_body(pLwc, last_html_str.c_str());
			}
            refresh_html_body = 0;
        }
    }
    void set_online(bool b) {
        container.set_online(b);
    }
    void on_remtex_gpu_loaded(unsigned long name_hash) {
        if (container.need_update_on_remtex_change(name_hash)) {
            redraw_fbo();
        }
    }
    void set_client_size(int client_width, int client_height) {
        this->client_width = client_width;
        this->client_height = client_height;
        container.set_client_size(client_width, client_height);
    }
    bool over_element(float nx, float ny) {
        if (doc) {
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height + scroll_y + scroll_dy));
            auto over_el = doc->root()->get_element_by_point(x, y, x, y);
            while (over_el) {
                const auto over_el_bg = over_el->get_background();
                if (strcmp(over_el->get_tagName(), "img") == 0) {
                    return true;
                }
                if (strcmp(over_el->get_tagName(), "html") != 0
                    && strcmp(over_el->get_tagName(), "body") != 0
                    && over_el_bg
                    && over_el_bg->m_color.alpha) {
                    return true;
                }
                over_el = over_el->parent();
            }
        }
        return false;
    }
    void execute_anchor_click(const char* url) {
        container.on_anchor_click_ex(url, doc->root(), false);
    }
    void add_touch_rect(float x, float y, float z, float width, float height, float extend_width, float extend_height, const mat4x4 view, const mat4x4 proj) {
        double start = lwtimepoint_now_seconds();
        for (size_t i = 0; i < touch_rect.size(); i++) {
            if (start - touch_rect[i].start > 1.0) {
                touch_rect[i].start = start;
                touch_rect[i].x = x;
                touch_rect[i].y = y;
                touch_rect[i].z = z;
                touch_rect[i].width = width;
                touch_rect[i].height = height;
                touch_rect[i].extend_width = extend_width;
                touch_rect[i].extend_height = extend_height;
                mat4x4_dup(touch_rect[i].view, view);
                mat4x4_dup(touch_rect[i].proj, proj);
                return;
            }
        }
        LOGE("touch_rect capacity exceeded.");
    }
    int get_touch_rect_count() const {
        return static_cast<int>(touch_rect.size());
    }
    void get_touch_rect(int index, double* start, float* x, float* y, float* z, float* width, float* height, float* extend_width, float* extend_height, mat4x4 view, mat4x4 proj) const {
        *start = touch_rect[index].start;
        *x = touch_rect[index].x;
        *y = touch_rect[index].y + scroll_y;
        *z = touch_rect[index].z;
        *width = touch_rect[index].width;
        *height = touch_rect[index].height;
        *extend_width = touch_rect[index].extend_width;
        *extend_height = touch_rect[index].extend_height;
        mat4x4_dup(view, touch_rect[index].view);
        mat4x4_dup(proj, touch_rect[index].proj);
    }
    void render_render_commands() { container.render_render_commands(pLwc, scroll_y + scroll_dy); }
    void update_on_render_thread() {
        const float retraction = 3.0f;
        if (mouse_dragging == false) {
            if (scroll_y < 0) {
                // trying to scroll to top of scroll view
                // interpolate scroll_y to '0'
                scroll_y += (0 - scroll_y) / retraction;
                if (scroll_y > 0) {
                    scroll_y = 0;
                }
            } else if (scroll_y > document_size.height - client_height) {
                // trying to scroll to bottom of scroll view
                // interpolate scroll_y to 'document_size.height - client_height'
                scroll_y += ((document_size.height - client_height) - scroll_y) / retraction;
                if (scroll_y < document_size.height - client_height) {
                    scroll_y = static_cast<float>(document_size.height - client_height);
                }
            }
        }
    }
private:
    LWHTMLUI();
    LWHTMLUI(const LWHTMLUI&);
    LWCONTEXT* pLwc;
    litehtml::context browser_context;
    //litehtml::text_container container;
    litehtml::render_command_container container;
    litehtml::document::ptr doc;
    int client_width;
    int client_height;
    litehtml::element::ptr last_lbutton_down_element;
    std::string next_html_path;
    int refresh_html_body;
    LWMUTEX parsing_mutex;
    std::string last_html_str;
    struct TOUCHRECT {
        double start;
        float x, y, z, width, height, extend_width, extend_height;
        mat4x4 view, proj;
        TOUCHRECT() : start(0), x(0), y(0), z(0), width(0), height(0), extend_width(0), extend_height(0) {
            mat4x4_identity(view);
            mat4x4_identity(proj);
        }
    };
    std::vector<TOUCHRECT> touch_rect;
    float scroll_y;
    float scroll_dy;
    bool mouse_down;
    float mouse_down_nx;
    float mouse_down_ny;
    bool mouse_dragging;
    litehtml::size document_size;
};

void* htmlui_new(LWCONTEXT* pLwc) {
    return new LWHTMLUI(pLwc, pLwc->viewport_width, pLwc->viewport_height);
}

void htmlui_destroy(void** c) {
    LWHTMLUI** htmlui = (LWHTMLUI**)c;
    delete(*htmlui);
    *htmlui = 0;
}

void htmlui_load_render_draw(void* c, const char* html_path) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    if (htmlui->load_page(html_path)) {
        htmlui->render_page();
        htmlui->draw();
    }
}

void htmlui_load_redraw_fbo(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->redraw_fbo();
}

void htmlui_load_render_draw_body(void* c, const char* html_body) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_body(html_body);
    htmlui->render_page();
    htmlui->draw();
}

void htmlui_on_lbutton_down(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_lbutton_down(nx, ny);
}

void htmlui_on_lbutton_up(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_lbutton_up(nx, ny);
}

void htmlui_on_over(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_over(nx, ny);
}

void htmlui_set_next_html_path(void* c, const char* html_path) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_next_html_path(html_path);
}

void htmlui_load_next_html_path(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_next_html_path();
}

void htmlui_set_refresh_html_body(void* c, int v) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_refresh_html_body(v);
}

void htmlui_load_next_html_body(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_next_html_body();
}

void htmlui_clear_loop(void* c, const char* loop_name) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->clear_loop(loop_name);
}

void htmlui_set_loop_key_value(void* c, const char* loop_name, const char* key, const char* value) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
	if (c == 0 || loop_name == 0 || key == 0 || value == 0) {
		LOGE("htmlui_set_loop_key_value error!");
		return;
	}
    htmlui->set_loop_key_value(loop_name, key, value);
}

void htmlui_set_online(void* c, int b) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_online(b ? true : false);
}

void htmlui_update_country_data(const LWCONTEXT* pLwc, void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui_clear_loop(htmlui, "country");
    int items_per_page = 20;
    for (int i = pLwc->country_page * items_per_page;
         i < LWMIN((pLwc->country_page + 1) * items_per_page, pLwc->country_array.count);
         i++) {
        char img_src[64];
        sprintf(img_src, "atlas/flags-mini/%s.png", pLwc->country_array.first[i].code);
        char* p = img_src;
        for (; *p; ++p) {
            *p = tolower(*p);
        }
        htmlui_set_loop_key_value(htmlui, "country", "name", pLwc->country_array.first[i].name);
        htmlui_set_loop_key_value(htmlui, "country", "img_src", img_src);
    }
}

void htmlui_on_remtex_gpu_loaded(void* c, unsigned int name_hash) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_remtex_gpu_loaded(name_hash);
}

void htmlui_set_client_size(void* c, int client_width, int client_height) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_client_size(client_width, client_height);
}

int htmlui_over_element(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    return htmlui->over_element(nx, ny) ? 1 : 0;
}

int htmlui_get_refresh_html_body(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    return htmlui->get_refresh_html_body();
}

void htmlui_execute_anchor_click(void* c, const char* url) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->execute_anchor_click(url);
}

void htmlui_add_touch_rect(void* c, float x, float y, float z, float width, float height, float extend_width, float extend_height, const mat4x4 view, const mat4x4 proj) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->add_touch_rect(x, y, z, width, height, extend_width, extend_height, view, proj);
}

int htmlui_get_touch_rect_count(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    return htmlui->get_touch_rect_count();
}

void htmlui_get_touch_rect(void* c, int index, double* start, float* x, float* y, float* z, float* width, float* height, float* extend_width, float* extend_height, mat4x4 view, mat4x4 proj) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->get_touch_rect(index, start, x, y, z, width, height, extend_width, extend_height, view, proj);
}

void htmlui_render_render_commands(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->render_render_commands();
}

void htmlui_update_on_render_thread(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->update_on_render_thread();
}
