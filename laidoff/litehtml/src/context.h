#pragma once
#include "stylesheet.h"

namespace litehtml
{
	class context
	{
    public:
        typedef std::vector<std::string> loop_var_string_array;
        typedef std::map<std::string, loop_var_string_array> loop_var_value;
        typedef std::map<std::string, loop_var_value> loop_var_map;
	
		void			load_master_stylesheet(const tchar_t* str);
		
        litehtml::css&	master_css() { return m_master_css; }

        context() : m_loop_began(false) {}
        int begin_loop(const tchar_t* loop_name);
        void end_loop(const tchar_t* loop_name);
        void next_loop_element(const tchar_t* loop_name);
        const char* get_loop_var(const char* key) const;

        bool is_in_loop() const { return m_loop_began; }
        void set_loop_var_map(const loop_var_map& m) { m_loop_var_map = m; }
        void set_loop_var_map_entry(const std::string& loop_name, const loop_var_value& var_value) { m_loop_var_map[loop_name] = var_value; }
        void set_loop_key_value(const std::string& loop_name, const std::string& key, const std::string& value) { m_loop_var_map[loop_name][key].push_back(value); }
        void clear_loop(const std::string& loop_name) { m_loop_var_map[loop_name].clear(); }
        void clear_all_loops() { m_loop_var_map.clear(); }
    private:
        litehtml::css	m_master_css;
        loop_var_map m_loop_var_map;
        loop_var_map::const_iterator m_loop_var_map_it;
        size_t m_loop_var_map_index;
        bool m_loop_began;

	};
}