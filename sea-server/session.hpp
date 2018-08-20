#pragma once
#include <string>
#include <unordered_map>
namespace ss {
    class session {
    public:
        void register_key(const char* account_id, const char* key_str, int key_str_len);
        const char* get_key(const char* account_id) const;
    private:
        std::unordered_map<std::string, std::string> session_map_;
    };
}