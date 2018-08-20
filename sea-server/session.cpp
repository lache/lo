#include "session.hpp"
using namespace ss;

void session::register_key(const char* account_id, const char* key_str, int key_str_len) {
    session_map_[account_id] = std::string(key_str, key_str_len);
}

const char* session::get_key(const char* account_id) const {
    auto cit = session_map_.find(account_id);
    if (cit != session_map_.end()) {
        return cit->second.c_str();
    } else {
        return nullptr;
    }
}
