#define _CRT_SECURE_NO_WARNINGS
#include "html.h"
#include "context.h"
#include "stylesheet.h"


void litehtml::context::load_master_stylesheet( const tchar_t* str )
{
	media_query_list::ptr media;

	m_master_css.parse_stylesheet(str, 0, std::shared_ptr<litehtml::document>(), media_query_list::ptr());
	m_master_css.sort_selectors();
}

int litehtml::context::begin_loop(const tchar_t* loop_name) {
    auto it = m_loop_var_map.find(loop_name);
    if (it != m_loop_var_map.end()) {
        auto it2 = it->second.begin();
        if (it2 != it->second.end()) {
            m_loop_var_map_it = it;
            m_loop_var_map_index = 0;
            m_loop_began = true;
            return it2->second.size();
        }
    }
    return 0;
}

void litehtml::context::end_loop(const tchar_t* loop_name) {
    if (m_loop_began) {
        if (m_loop_var_map_it->first == loop_name) {
            m_loop_began = false;
        }
    }
}

void litehtml::context::next_loop_element(const tchar_t* loop_name) {
    if (m_loop_began) {
        if (m_loop_var_map_it->first == loop_name) {
            m_loop_var_map_index++;
        }
    }
}

const char* litehtml::context::get_loop_var(const char* key) const {
    // trim left & right whitespaces of 'key'
    while (*key == '\t'
           || *key == '\r'
           || *key == '\n'
           || *key == '\f'
           || *key == ' ') {
        key++;
    }
    size_t key_len = strlen(key);
    while (*(key + key_len - 1) == '\t'
           || *(key + key_len - 1) == '\r'
           || *(key + key_len - 1) == '\n'
           || *(key + key_len - 1) == '\f'
           || *(key + key_len - 1) == ' ') {
        key_len--;
    }
    // key should have format '{{key-name}}'
    if (key_len < 5) { // minimum 5 characters
        return key;
    }
    if (key_len > 63 + 4) {
        return key;
    }
    char key_copy[64];
    strncpy(key_copy, key + 2, key_len - 4);
    key_copy[key_len - 4] = 0;
    auto it = m_loop_var_map_it->second.find(key_copy);
    if (it != m_loop_var_map_it->second.end()) {
        return it->second[m_loop_var_map_index].c_str();
    }
    return key;
}