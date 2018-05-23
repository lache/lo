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

litehtml::text_container::text_container(LWCONTEXT* pLwc, int w, int h)
    : pLwc(pLwc), client_width(w), client_height(h), default_font_size(36) {
}

litehtml::text_container::~text_container() {
}

litehtml::uint_ptr litehtml::text_container::create_font(const litehtml::tchar_t * faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics * fm) {
    LOGIx("create_font: faceName=%s, size=%d, weight=%d\n", faceName, size, weight);
    size_t font_idx = font_sizes.size();
    font_sizes.push_back(size);
    fm->height = static_cast<int>(roundf(size * 0.8f * pLwc->height / 720.f));
    fm->descent = static_cast<int>(roundf(size * 0.1f * pLwc->height / 720.f));
    return litehtml::uint_ptr(font_idx);// litehtml::uint_ptr();
}

void litehtml::text_container::delete_font(litehtml::uint_ptr hFont) {
}

static float conv_size_x(const LWCONTEXT* pLwc, int x) {
    return 2 * ((float)x / pLwc->width * pLwc->aspect_ratio);
}

static float conv_size_y(const LWCONTEXT* pLwc, int y) {
    return 2 * ((float)y / pLwc->height);
}

static float conv_coord_x(const LWCONTEXT* pLwc, int x) {
    return -pLwc->aspect_ratio + conv_size_x(pLwc, x);
}

static float conv_coord_y(const LWCONTEXT* pLwc, int y) {
    return 1.0f - conv_size_y(pLwc, y);
}

static void fill_text_block(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block, int x, int y, const char* text, int size, const litehtml::web_color& color) {
    text_block->text_block_width = 999.0f;// 2.00f * aspect_ratio;
    LOGIx("font size: %d", size);
    text_block->text_block_line_height = size / 72.0f;
    text_block->size = size / 72.0f;
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, (255 - color.red) / 255.0f, (255 - color.green) / 255.0f, (255 - color.blue) / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    text_block->text = text;
    text_block->text_bytelen = (int)strlen(text_block->text);
    text_block->begin_index = 0;
    text_block->end_index = text_block->text_bytelen;
    text_block->multiline = 1;
    text_block->text_block_x = conv_coord_x(pLwc, x);
    text_block->text_block_y = conv_coord_y(pLwc, y);
    text_block->align = LTBA_LEFT_BOTTOM;
}

int litehtml::text_container::text_width(const litehtml::tchar_t * text, litehtml::uint_ptr hFont) {
    LWTEXTBLOCK text_block;
    LWTEXTBLOCKQUERYRESULT query_result;
    int size = font_sizes[(int)(size_t)hFont];
    litehtml::web_color c;
    fill_text_block(pLwc, &text_block, 0, 0, text, size, c);
    render_query_only_text_block(pLwc, &text_block, &query_result);
    return static_cast<int>(query_result.total_glyph_width / (2 * pLwc->aspect_ratio) * pLwc->width);
}

void litehtml::text_container::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t * text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position & pos) {
    //wprintf(_t("draw_text: '%s' (x=%d,y=%d,color=0x%02X%02X%02X|%02X)\n"), text, pos.x, pos.y, color.red, color.green, color.blue, color.alpha);
    int size = font_sizes[(int)(size_t)hFont];
    LWTEXTBLOCK text_block;
    fill_text_block(pLwc, &text_block, pos.x, pos.y, text, size, color);
    render_text_block(pLwc, &text_block);
}

int litehtml::text_container::pt_to_px(int pt) {
    return static_cast<int>(roundf(pt * 3.6f * pLwc->width / 640.0f));
}

int litehtml::text_container::get_default_font_size() const {
    return default_font_size;
}

const litehtml::tchar_t * litehtml::text_container::get_default_font_name() const {
    return _t("Arial");
}

void litehtml::text_container::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker & marker) {
    LOGIx("draw_list_marker");
    int size = font_sizes[0];
    LWTEXTBLOCK text_block;
    fill_text_block(pLwc, &text_block, marker.pos.x + marker.pos.width, marker.pos.y + marker.pos.height, "*", size, marker.color);
    render_text_block(pLwc, &text_block);
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

bool has_separate_alpha_texture(const char* remtex_name) {
    return strncmp(remtex_name + strlen(remtex_name) - 2, "-a", 2) == 0
        || strncmp(remtex_name + strlen(remtex_name) - 5, "-amip", 5) == 0;
}

void separate_alpha_texture(const char* src, char* alpha_src, size_t max_alpha_src_len) {
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
    float scale = pLwc->width / 640.0f;
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
            conv_coord_x(pLwc, bg.border_box.x),
            conv_coord_y(pLwc, bg.border_box.y),
            conv_size_x(pLwc, bg.border_box.width),
            conv_size_y(pLwc, bg.border_box.height),
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
            conv_coord_x(pLwc, bg.border_box.x),
            conv_coord_y(pLwc, bg.border_box.y),
            conv_size_x(pLwc, bg.border_box.width),
            conv_size_y(pLwc, bg.border_box.height),
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
                                conv_size_x(pLwc, bg.border_box.width),
                                conv_coord_x(pLwc, bg.border_box.x),
                                conv_coord_y(pLwc, bg.border_box.y),
                                1.0f,
                                LVT_LEFT_TOP_ANCHORED_SQUARE);
    } else if (show_test_image == 4 && remtex_id) {
        if (alpha_remtex_id == 0) {
            render_solid_vb_ui_flip_y_uv_shader(
                pLwc,
                conv_coord_x(pLwc, bg.border_box.x),
                conv_coord_y(pLwc, bg.border_box.y),
                conv_size_x(pLwc, bg.border_box.width),
                conv_size_y(pLwc, bg.border_box.height),
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
                conv_coord_x(pLwc, bg.border_box.x),
                conv_coord_y(pLwc, bg.border_box.y),
                conv_size_x(pLwc, bg.border_box.width),
                conv_size_y(pLwc, bg.border_box.height),
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
            conv_coord_x(pLwc, bg.border_box.x),
            conv_coord_y(pLwc, bg.border_box.y),
            conv_size_x(pLwc, bg.border_box.width),
            conv_size_y(pLwc, bg.border_box.height),
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
        conv_coord_x(pLwc, x),
        conv_coord_y(pLwc, y),
        conv_size_x(pLwc, w),
        conv_size_y(pLwc, h),
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
    LOGIx("on_anchor_click: %s", url);
    if (strncmp(url, "script:", strlen("script:")) == 0) {
        script_evaluate_with_name_async(pLwc,
                                        url + strlen("script:"), // remove 'script:' prefix
                                        strlen(url + strlen("script:")),
                                        "on_anchor_click");
    } else {
        const char* path_prefix = ASSETS_BASE_PATH "html" PATH_SEPARATOR;
        char path[1024] = { 0, };
        strcat(path, path_prefix);
        strcat(path, url);
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
}
