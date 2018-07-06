#include "text_container.h"
#include <stdio.h>
#include <locale>
#include "lwgl.h"
#include "lwtextblock.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "lwlog.h"
#include "htmlui.h"
#include "lwtcpclient.h"
#include "el_luascript.h"
#include "script.h"
#include "lwatlassprite.h"
#include "lwttl.h"
#include "remtex.h"
#include "logic.h"

extern "C" unsigned long hash(const unsigned char *str);

litehtml::text_container::text_container(LWCONTEXT* pLwc, int w, int h)
    : pLwc(pLwc)
    , font_handle_seq(0) {
    set_client_size(w, h);
}

litehtml::text_container::~text_container() {
}

litehtml::uint_ptr litehtml::text_container::create_font(const litehtml::tchar_t * faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics * fm) {
    font_handle_seq++;
    litehtml::uint_ptr font_handle = reinterpret_cast<litehtml::uint_ptr>(font_handle_seq);
    LOGIx("create_font: faceName=%s, size=%d, weight=%d --> font handle %d", faceName, size, weight, font_handle);
    font_sizes[font_handle] = size;
    if (client_aspect_ratio > 1) {
        //fm->height = static_cast<int>(roundf(size * 0.8f * client_height / 720.0f));
        //fm->descent = static_cast<int>(roundf(size * 0.1f * client_height / 720.0f));
    } else {
        //fm->height = static_cast<int>(roundf(size * 0.8f * client_width / 1280.0f));
        //fm->descent = static_cast<int>(roundf(size * 0.1f * client_width / 1280.0f));
    }
    fm->height = static_cast<int>(font_sizes[font_handle] / 5.5f);// static_cast<int>(roundf(size * 0.8f * client_height / 720.0f));
    fm->descent = static_cast<int>(fm->height / 4.5f);
    //fm->ascent = fm->height / 4;
    return font_handle;
}

void litehtml::text_container::delete_font(litehtml::uint_ptr hFont) {
    LOGIx("delete_font: font handle %d", hFont);
    font_sizes.erase(hFont);
}

void litehtml::text_container::fill_text_block(LWTEXTBLOCK* text_block, int x, int y, const char* text, int size, const litehtml::web_color& color) {
    text_block->text_block_width = 999.0f;
    LOGIx("font size: %d", size);
    text_block->text_block_line_height = static_cast<float>(size);
    text_block->size = static_cast<float>(size);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, (255 - color.red) / 255.0f, (255 - color.green) / 255.0f, (255 - color.blue) / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    text_block->text = text;
    text_block->text_bytelen = (int)strlen(text_block->text);
    text_block->begin_index = 0;
    text_block->end_index = text_block->text_bytelen;
    text_block->multiline = 1;
    text_block->text_block_x = conv_coord_x(x);
    text_block->text_block_y = conv_coord_y(y);
    text_block->align = LTBA_LEFT_BOTTOM;
}

int litehtml::text_container::text_width(const litehtml::tchar_t * text, litehtml::uint_ptr hFont) {
    LWTEXTBLOCK text_block;
    LWTEXTBLOCKQUERYRESULT query_result;
    int size = font_sizes[hFont];
    litehtml::web_color c;
    fill_text_block(&text_block, 0, 0, text, size, c);
    lwtextblock_query_only_text_block(pLwc, &text_block, &query_result);
    //return static_cast<int>(query_result.total_glyph_width / (2 * client_rt_x) * client_width);
    return static_cast<int>(query_result.total_glyph_width);
}

void litehtml::text_container::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t * text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position & pos) {
    //wprintf(_t("draw_text: '%s' (x=%d,y=%d,color=0x%02X%02X%02X|%02X)\n"), text, pos.x, pos.y, color.red, color.green, color.blue, color.alpha);
    int size = font_sizes[hFont];
    LWTEXTBLOCK text_block;
    fill_text_block(&text_block, pos.x, pos.y, text, size, color);
    //render_text_block(pLwc, &text_block);
    render_text_block_two_pass_color(pLwc, &text_block);
}

int litehtml::text_container::pt_to_px(int pt) {
    //return static_cast<int>(roundf(pt * 3.6f * client_width / 640.0f));
    return pt * 3;
}

int litehtml::text_container::get_default_font_size() const {
    return default_font_size;
}

const litehtml::tchar_t * litehtml::text_container::get_default_font_name() const {
    return _t("Arial");
}

void litehtml::text_container::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker & marker) {
    LOGIx("draw_list_marker");
    if (font_sizes.begin() != font_sizes.end()) {
        int size = font_sizes.begin()->second;
        LWTEXTBLOCK text_block;
        fill_text_block(&text_block, marker.pos.x + marker.pos.width, marker.pos.y + marker.pos.height, "*", size, marker.color);
        render_text_block(pLwc, &text_block);
    }
}

void litehtml::text_container::load_image(const litehtml::tchar_t * src, const litehtml::tchar_t * baseurl, bool redraw_on_ready) {
    //wprintf(_t("load_image: src=%s,baseurl=%s,redraw_on_ready=%d\n"), src, baseurl, redraw_on_ready);
}

static LWATLASSPRITEPTR atlas_sprite_ptr_from_url(const LWCONTEXT* pLwc, const std::string& src) {
    LWATLASSPRITEPTR atlas_sprite_ptr = { 0, 0 };
    const static char* atlas_prefix = "atlas/";
    const static size_t atlas_prefix_len = strlen(atlas_prefix);
    if (strncmp(src.c_str(), atlas_prefix, atlas_prefix_len) == 0) {
        const char* atlas_name_beg = src.c_str() + atlas_prefix_len;
        const char* atlas_name_end = strchr(src.c_str() + atlas_prefix_len, '/');
        const char* sprite_name_end = strrchr(src.c_str(), '.');
        char atlas_name[64];
        strncpy(atlas_name, atlas_name_beg, atlas_name_end - atlas_name_beg);
        atlas_name[atlas_name_end - atlas_name_beg] = 0;
        char sprite_name[64];
        strncpy(sprite_name, atlas_name_end + 1, sprite_name_end - (atlas_name_end + 1));
        sprite_name[sprite_name_end - (atlas_name_end + 1)] = 0;
        atlas_sprite_ptr = atlas_name_sprite_name(pLwc, atlas_name, sprite_name);
    }
    return atlas_sprite_ptr;
}

static bool has_separate_alpha_texture(const char* remtex_name) {
    return strncmp(remtex_name + strlen(remtex_name) - 2, "-a", 2) == 0
        || strncmp(remtex_name + strlen(remtex_name) - 5, "-amip", 5) == 0;
}

static void separate_alpha_texture(const char* src, char* alpha_src, size_t max_alpha_src_len) {
    size_t copy_len = LWMIN(strlen(src) - 4, max_alpha_src_len - 1);
    strncpy(alpha_src, src, copy_len);
    alpha_src[copy_len] = 0;
    if (copy_len + strlen("_alpha.png") <= max_alpha_src_len) {
        strcat(alpha_src, "_alpha.png");
    } else {
        abort();
    }
}

void litehtml::text_container::get_image_size(const litehtml::tchar_t * src, const litehtml::tchar_t * baseurl, litehtml::size & sz) {
    LOGIx("get_image_size: src=%s,baseurl=%s", src, baseurl);
    // [1] check for 'atlas type' image
    LWATLASSPRITEPTR atlas_sprite_ptr = atlas_sprite_ptr_from_url(pLwc, src);
    float scale = client_width / 640.0f;
    if (atlas_sprite_ptr.sprite) {
        sz.width = static_cast<int>(roundf(atlas_sprite_ptr.sprite->width * scale));
        sz.height = static_cast<int>(roundf(atlas_sprite_ptr.sprite->height * scale));
    } else {
        // [2] check for 'remtex' type image
        int valid_remtex = 0;
        char remtex_name[64];
        unsigned long name_hash = remtex_name_hash_from_url(src, &valid_remtex, remtex_name, ARRAY_SIZE(remtex_name));
        if (valid_remtex) {
            remtex_name_hash_set.insert(name_hash);
            int remtex_id = remtex_preload(pLwc->remtex, remtex_name);
            if (remtex_id >= 0) {
                if (remtex_gpu_loaded(pLwc->remtex, remtex_id)) {
                    // already loaded
                    int remtex_w, remtex_h;
                    if (remtex_width_height(pLwc->remtex, remtex_id, &remtex_w, &remtex_h)) {
                        sz.width = static_cast<int>(roundf(remtex_w * scale));
                        sz.height = static_cast<int>(roundf(remtex_h * scale));
                    }
                } else {
                    // not loaded...
                }
            }
            // check for separate alpha texture and load if exists
            if (has_separate_alpha_texture(remtex_name)) {
                int valid_alpha_remtex = 0;
                char alpha_remtex_name[64];
                char alpha_src[128];
                separate_alpha_texture(src, alpha_src, ARRAY_SIZE(alpha_src));
                unsigned long alpha_name_hash = remtex_name_hash_from_url(alpha_src, &valid_alpha_remtex, alpha_remtex_name, ARRAY_SIZE(alpha_remtex_name));
                if (valid_alpha_remtex) {
                    remtex_name_hash_set.insert(alpha_name_hash);
                    remtex_preload(pLwc->remtex, alpha_remtex_name);
                }
            }
        } else {
            // [3] temporary test image
            sz.width = static_cast<int>(roundf(180 * scale));
            sz.height = static_cast<int>(roundf(180 * scale));
        }
    }
}

void litehtml::text_container::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint & bg) {
    // ignore body background-color (only applied on browser-testing phase)
    if (bg.is_root) {
        return;
    }
    if (bg.image == "img_trans.gif") {
        // 1x1 transparent dummy image used for CSS image slicing method
        return;
    }

    int lae = LAE_TTL_TITLE;
    int lae_alpha = LAE_TTL_TITLE_ALPHA;

    int show_test_image = 0;

    if (bg.image.length()) {
        LOGIx("draw_background [IMAGE]: x=%d,y=%d,w=%d,h=%d,clipbox=%d/%d/%d/%d,position_xy=%d/%d,image_size=%d/%d,color=0x%02X%02X%02X|%02X,image=%s,baseurl=%s",
              bg.border_box.x,
              bg.border_box.y,
              bg.border_box.width,
              bg.border_box.height,
              bg.clip_box.x,
              bg.clip_box.y,
              bg.clip_box.width,
              bg.clip_box.height,
              bg.position_x,
              bg.position_y,
              bg.image_size.width,
              bg.image_size.height,
              bg.color.red,
              bg.color.green,
              bg.color.blue,
              bg.color.alpha,
              bg.image.c_str(),
              bg.baseurl.c_str());
        show_test_image = 1;

        if (bg.image == "slice-test.png") {
            show_test_image = 2;
            lae = LAE_SLICE_TEST;
        }
    }

    // check 'bg.image' is atlas sprite
    LWATLASSPRITEPTR atlas_sprite_ptr = atlas_sprite_ptr_from_url(pLwc, bg.image);
    GLuint remtex_id = 0, alpha_remtex_id = 0;
    if (atlas_sprite_ptr.sprite != 0) {
        lae = atlas_sprite_lae(&atlas_sprite_ptr);
        lae_alpha = atlas_sprite_alpha_lae(&atlas_sprite_ptr);
        show_test_image = 3;
    } else {
        // check 'bg.image' is remtex
        int valid_remtex = 0;
        char remtex_name[64];
        unsigned long name_hash = remtex_name_hash_from_url(bg.image.c_str(), &valid_remtex, remtex_name, ARRAY_SIZE(remtex_name));
        if (valid_remtex) {
            show_test_image = 4;
            remtex_id = remtex_load_tex(pLwc->remtex, remtex_name);
            // check for separate alpha texture and load if exists
            if (has_separate_alpha_texture(remtex_name)) {
                char alpha_src[128];
                separate_alpha_texture(bg.image.c_str(), alpha_src, ARRAY_SIZE(alpha_src));

                int valid_alpha_remtex = 0;
                char alpha_remtex_name[64];
                unsigned long alpha_name_hash = remtex_name_hash_from_url(alpha_src, &valid_alpha_remtex, alpha_remtex_name, ARRAY_SIZE(alpha_remtex_name));
                if (valid_alpha_remtex) {
                    alpha_remtex_id = remtex_load_tex(pLwc->remtex, alpha_remtex_name);
                }
            }
        }
    }

    if (show_test_image == 1) {
        lazy_tex_atlas_glBindTexture(pLwc, lae);
        lazy_tex_atlas_glBindTexture(pLwc, lae_alpha);
        render_solid_vb_ui_alpha(
            pLwc,
            conv_coord_x(bg.border_box.x),
            conv_coord_y(bg.border_box.y),
            conv_size_x(bg.border_box.width),
            conv_size_y(bg.border_box.height),
            show_test_image ? pLwc->tex_atlas[lae] : 0,
            show_test_image ? pLwc->tex_atlas[lae_alpha] : 0,
            LVT_LEFT_TOP_ANCHORED_SQUARE,
            bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
            show_test_image ? 1.0f : bg.color.red / 255.0f,
            show_test_image ? 1.0f : bg.color.green / 255.0f,
            show_test_image ? 1.0f : bg.color.blue / 255.0f,
            show_test_image ? 0.0f : 1.0f
        );
    } else if (show_test_image == 2) {
        float offset_x = (float)(bg.border_box.x - bg.position_x) / bg.image_size.width;
        float offset_y = (float)(bg.border_box.y - bg.position_y) / bg.image_size.height;
        float uv_offset[] = { offset_x, offset_y };
        float uv_scale[] = { (float)bg.border_box.width / bg.image_size.width, (float)bg.border_box.height / bg.image_size.height };
        lazy_tex_atlas_glBindTexture(pLwc, lae);
        render_solid_vb_ui_uv_shader_rot(
            pLwc,
            conv_coord_x(bg.border_box.x),
            conv_coord_y(bg.border_box.y),
            conv_size_x(bg.border_box.width),
            conv_size_y(bg.border_box.height),
            show_test_image ? pLwc->tex_atlas[lae] : 0,
            LVT_LEFT_TOP_ANCHORED_SQUARE,
            bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
            show_test_image ? 1.0f : bg.color.red / 255.0f,
            show_test_image ? 1.0f : bg.color.green / 255.0f,
            show_test_image ? 1.0f : bg.color.blue / 255.0f,
            show_test_image ? 0.0f : 1.0f,
            uv_offset,
            uv_scale,
            LWST_DEFAULT,
            0
        );
    } else if (show_test_image == 3) {
        render_atlas_sprite_ptr(pLwc,
                                atlas_sprite_ptr.sprite,
                                (LW_ATLAS_ENUM)lae,
                                (LW_ATLAS_ENUM)lae_alpha,
                                conv_size_x(bg.border_box.width),
                                conv_coord_x(bg.border_box.x),
                                conv_coord_y(bg.border_box.y),
                                1.0f,
                                LVT_LEFT_TOP_ANCHORED_SQUARE);
    } else if (show_test_image == 4 && remtex_id) {
        if (alpha_remtex_id == 0) {
            render_solid_vb_ui_flip_y_uv_shader(
                pLwc,
                conv_coord_x(bg.border_box.x),
                conv_coord_y(bg.border_box.y),
                conv_size_x(bg.border_box.width),
                conv_size_y(bg.border_box.height),
                remtex_id,
                LVT_LEFT_TOP_ANCHORED_SQUARE,
                bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
                show_test_image ? 1.0f : bg.color.red / 255.0f,
                show_test_image ? 1.0f : bg.color.green / 255.0f,
                show_test_image ? 1.0f : bg.color.blue / 255.0f,
                show_test_image ? 0.0f : 1.0f,
                0,
                LWST_DEFAULT
            );
        } else {
            render_solid_vb_ui_alpha(
                pLwc,
                conv_coord_x(bg.border_box.x),
                conv_coord_y(bg.border_box.y),
                conv_size_x(bg.border_box.width),
                conv_size_y(bg.border_box.height),
                remtex_id,
                alpha_remtex_id,
                LVT_LEFT_TOP_ANCHORED_SQUARE,
                bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
                show_test_image ? 1.0f : bg.color.red / 255.0f,
                show_test_image ? 1.0f : bg.color.green / 255.0f,
                show_test_image ? 1.0f : bg.color.blue / 255.0f,
                show_test_image ? 0.0f : 1.0f
            );
        }
    } else {
        render_solid_vb_ui_flip_y_uv_shader(
            pLwc,
            conv_coord_x(bg.border_box.x),
            conv_coord_y(bg.border_box.y),
            conv_size_x(bg.border_box.width),
            conv_size_y(bg.border_box.height),
            show_test_image ? pLwc->tex_atlas[lae] : 0,
            LVT_LEFT_TOP_ANCHORED_SQUARE,
            bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
            show_test_image ? 1.0f : bg.color.red / 255.0f,
            show_test_image ? 1.0f : bg.color.green / 255.0f,
            show_test_image ? 1.0f : bg.color.blue / 255.0f,
            show_test_image ? 0.0f : 1.0f,
            0,
            LWST_DEFAULT
        );
    }
}

void litehtml::text_container::draw_border_rect(const litehtml::border& border, int x, int y, int w, int h, LW_VBO_TYPE lvt, const litehtml::web_color& color) const {
    if (w <= 0 || h <= 0) {
        return;
    }
    if (border.style == border_style_none || border.style == border_style_hidden) {
        return;
    }
    render_solid_vb_ui_flip_y_uv_shader(
        pLwc,
        conv_coord_x(x),
        conv_coord_y(y),
        conv_size_x(w),
        conv_size_y(h),
        0,
        lvt,
        color.alpha / 255.0f,
        color.red / 255.0f,
        color.green / 255.0f,
        color.blue / 255.0f,
        1.0f,
        0,
        LWST_DEFAULT
    );
}

void litehtml::text_container::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders & borders, const litehtml::position & draw_pos, bool root) {
    LOGIx("draw_borders: x=%d y=%d w=%d h=%d Lw=%d Rw=%d Tw=%d Bw=%d root=%d",
          draw_pos.x,
          draw_pos.y,
          draw_pos.width,
          draw_pos.height,
          borders.left.width,
          borders.right.width,
          borders.top.width,
          borders.bottom.width,
          root);
    // left border
    draw_border_rect(borders.left, draw_pos.x, draw_pos.y, borders.left.width, draw_pos.height, LVT_LEFT_TOP_ANCHORED_SQUARE, borders.left.color);
    // right border
    draw_border_rect(borders.right, draw_pos.x + draw_pos.width, draw_pos.y, borders.right.width, draw_pos.height, LVT_RIGHT_TOP_ANCHORED_SQUARE, borders.right.color);
    // top border
    draw_border_rect(borders.top, draw_pos.x, draw_pos.y, draw_pos.width, borders.top.width, LVT_LEFT_TOP_ANCHORED_SQUARE, borders.top.color);
    // bottom border
    draw_border_rect(borders.bottom, draw_pos.x, draw_pos.y + draw_pos.height, draw_pos.width, borders.bottom.width, LVT_LEFT_BOTTOM_ANCHORED_SQUARE, borders.bottom.color);
}

void litehtml::text_container::set_caption(const litehtml::tchar_t * caption) {
    //wprintf(_t("set_caption: %s\n"), caption);
}

void litehtml::text_container::set_base_url(const litehtml::tchar_t * base_url) {
    //wprintf(_t("set_base_url: %s\n"), base_url);
}

void litehtml::text_container::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr & el) {
    //printf("link\n");
}

void litehtml::text_container::on_anchor_click(const litehtml::tchar_t * url, const litehtml::element::ptr & el) {
    on_anchor_click_ex(url, el, true);
}

void litehtml::text_container::on_anchor_click_ex(const litehtml::tchar_t * url, const litehtml::element::ptr & el, bool add_touch_rect) {
    LOGIx("on_anchor_click_ex: %s", url);
    auto global_position = el->get_position();
    litehtml::element::ptr el_recursive = el;
    while (el_recursive->have_parent()) {
        el_recursive = el_recursive->parent();
        const auto& parent_position = el_recursive->get_position();
        global_position.x += parent_position.x;
        global_position.y += parent_position.y;
    }
    global_position += el->get_paddings();
    global_position += el->get_borders();
    LOGIx("%s: global position x=%d,y=%d,w=%d,h=%d", __func__, global_position.x, global_position.y, global_position.width, global_position.height);
    if (add_touch_rect) {
        mat4x4 view, proj;
        mat4x4_identity(view);
        logic_update_default_ui_proj_for_htmlui(client_width, client_height, proj);
        const float touch_rect_x = global_position.x + global_position.width / 2.0f;
        const float touch_rect_y = client_height - (global_position.y + global_position.height / 2.0f);
        const float extend = client_width / 450.0f * 10.0f; // 10 pixel extension when touching (reference height 450 px)
        htmlui_add_touch_rect(pLwc->htmlui,
                              touch_rect_x,
                              touch_rect_y,
                              0,
                              static_cast<float>(global_position.width),
                              static_cast<float>(global_position.height),
                              extend,
                              extend,
                              view,
                              proj);
    }
    if (strncmp(url, "script:", strlen("script:")) == 0) {
        script_evaluate_with_name_async(pLwc,
                                        url + strlen("script:"), // remove 'script:' prefix
                                        strlen(url + strlen("script:")),
                                        "on_anchor_click");
    } else {
        if (url[0] == '/' && online == false) {
            LOGI("Online URL anchor requested when online == false; change online to true.");
            online = true;
        }
        if (online) {
            if (pLwc->tcp_ttl) {
                // make a script executing this asynchronously in logic thread;
                // since lwttl_http_header() should be called in logic thread
                //
                // tcp_send_httpget(pLwc->tcp_ttl, url, lwttl_http_header(pLwc->ttl));
                //
                char scr[2048];
                sprintf(scr,
                        "local c = lo.script_context();"
                        "lo.tcp_send_httpget(c.tcp_ttl, \"%s\", lo.lwttl_http_header(c.ttl))",
                        url);
                script_evaluate_with_name_async(pLwc,
                                                scr,
                                                strlen(scr),
                                                "on_anchor_click 2");
            } else {
                LOGE("tcp_ttl null");
            }
        } else {
            char path[1024] = { 0, };
            const char* path_prefix = ASSETS_BASE_PATH "html" PATH_SEPARATOR;
            strcat(path, path_prefix);
            strcat(path, url);
            htmlui_set_next_html_path(pLwc->htmlui, path);
        }
    }
}

void litehtml::text_container::set_cursor(const litehtml::tchar_t * cursor) {
    //printf("set_cursor\n");
}

void litehtml::text_container::transform_text(litehtml::tstring & text, litehtml::text_transform tt) {
    //printf("transform_text\n");
}

void litehtml::text_container::import_css(litehtml::tstring & text, const litehtml::tstring & url, litehtml::tstring & baseurl) {
    //printf("import_css\n");
}

void litehtml::text_container::set_clip(const litehtml::position & pos, const litehtml::border_radiuses & bdr_radius, bool valid_x, bool valid_y) {
    //printf("set_clip\n");
}

void litehtml::text_container::del_clip() {
    //printf("del_clip\n");
}

void litehtml::text_container::get_client_rect(litehtml::position & client) const {
    client.x = 0;
    client.y = 0;
    client.width = client_width;
    client.height = client_height;
    //printf("get_client_rect\n");
}

std::shared_ptr<litehtml::element> litehtml::text_container::create_element(const litehtml::tchar_t * tag_name, const litehtml::string_map & attributes, const std::shared_ptr<litehtml::document>& doc) {
    if (strcmp(tag_name, "script") == 0) {
        auto ait = attributes.find("type");
        if (ait != attributes.cend() && ait->second == "text/x-lua") {
            return std::make_shared<litehtml::el_luascript>(doc, pLwc);
        }
    }
    return std::shared_ptr<litehtml::element>();
}

void litehtml::text_container::get_media_features(litehtml::media_features & media) const {
}

void litehtml::text_container::get_language(litehtml::tstring & language, litehtml::tstring & culture) const {
}

void litehtml::text_container::set_client_size(int client_width, int client_height) {
    this->client_width = client_width;
    this->client_height = client_height;
    this->client_aspect_ratio = (float)client_width / client_height;
    lwcontext_rt_corner(this->client_aspect_ratio, &client_rt_x, &client_rt_y);
    //if (client_width > client_height) {
        //default_font_size = static_cast<int>((72.0f * 2) * client_width / 450);
    //} else {
    default_font_size = static_cast<int>((72.0f * 2) * client_height / 800);
    //}
}
