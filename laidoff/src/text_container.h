#pragma once
#include "litehtml.h"
#include <vector>
#include <set>
#include "lwvbotype.h"

typedef struct _LWCONTEXT LWCONTEXT;
namespace litehtml {
	class text_container : public document_container {
	public:
		typedef std::map<std::wstring, litehtml::uint_ptr>	images_map;

	protected:

		images_map					m_images;
		litehtml::position::vector	m_clips;
		std::vector<int> font_sizes;

	public:
		text_container(LWCONTEXT* pLwc, int w, int h);
		virtual ~text_container();

		virtual litehtml::uint_ptr	create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm);
		virtual void				delete_font(litehtml::uint_ptr hFont);
		virtual int					text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont);
		virtual void				draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos);
		virtual int					pt_to_px(int pt);
		virtual int					get_default_font_size() const;
		virtual const litehtml::tchar_t*	get_default_font_name() const;
		virtual void				draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker);
		virtual void				load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready);
		virtual void				get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz);
		virtual void				draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg);
		virtual void				draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root);

		virtual	void				set_caption(const litehtml::tchar_t* caption);
		virtual	void				set_base_url(const litehtml::tchar_t* base_url);
		virtual void				link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el);
		virtual void				on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el);
		virtual	void				set_cursor(const litehtml::tchar_t* cursor);
		virtual	void				transform_text(litehtml::tstring& text, litehtml::text_transform tt);
		virtual void				import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl);
		virtual void				set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y);
		virtual void				del_clip();
		virtual void				get_client_rect(litehtml::position& client) const;
		virtual std::shared_ptr<litehtml::element>	create_element(const litehtml::tchar_t *tag_name,
			const litehtml::string_map &attributes,
			const std::shared_ptr<litehtml::document> &doc);

		virtual void				get_media_features(litehtml::media_features& media) const;
		virtual void				get_language(litehtml::tstring& language, litehtml::tstring & culture) const;

        void set_online(bool b) { online = b; }
        void clear_remtex_name_hash_set() { remtex_name_hash_set.clear(); }
        bool need_update_on_remtex_change(unsigned long name_hash) const {
            return remtex_name_hash_set.find(name_hash) != remtex_name_hash_set
                .end();
        }
        void set_client_size(int client_width, int client_height);
	private:
        void draw_border_rect(const litehtml::border& border, int x, int y, int w, int h, LW_VBO_TYPE lvt, const litehtml::web_color& color) const;
		LWCONTEXT * pLwc;
		int client_width;
		int client_height;
		int default_font_size;
        bool online;
        std::set<unsigned long> remtex_name_hash_set;
	};
}
