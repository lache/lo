#pragma once

#ifdef WIN32
#include <stdio.h>
#include <windows.h>
struct error_log_context {
    HANDLE hConsole;
    WORD saved_attributes;
};
static error_log_context prepare_error_log() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    return error_log_context{ hConsole, saved_attributes };
}
static void finish_error_log(error_log_context& elc) {
    SetConsoleTextAttribute(elc.hConsole, elc.saved_attributes);
}
#else
struct error_log_context {};
static error_log_context prepare_error_log() {}
static void finish_error_log(error_log_context& elc) {}
#endif

namespace ss {
    static std::string awesome_printf_helper(boost::format& f) {
        return boost::str(f);
    }

    template<class T, class... Args>
    std::string awesome_printf_helper(boost::format& f, T&& t, Args&&... args) {
        try {
            return awesome_printf_helper(f % std::forward<T>(t), std::forward<Args>(args)...);
        } catch (boost::io::bad_format_string& e) {
            auto elc = prepare_error_log();
            std::cerr << e.what() << std::endl;
            std::cerr << "format: " << f << ", T: " << t << std::endl;
            finish_error_log(elc);
            return "";
        }
    }

    template<typename... Arguments>
    void LOG_WITH_PREFIX(std::ostream& stream, const std::string& prefix, const std::string& fmt, Arguments&&... args) {
        try {
            stream << prefix;
            boost::format f(fmt);
            stream << awesome_printf_helper(f, std::forward<Arguments>(args)...) << std::endl;
        } catch (boost::io::bad_format_string& e) {
            auto elc = prepare_error_log();
            std::cerr << e.what() << std::endl;
            std::cerr << "fmt: " << fmt << std::endl;
            finish_error_log(elc);
        }
    }

    template<typename... Arguments>
    void LOG(std::ostream& stream, const std::string& fmt, Arguments&&... args) {
        LOG_WITH_PREFIX(stream, "", fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGI(const std::string& fmt, Arguments&&... args) {
        LOG(std::cout, fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGI_WITH_PREFIX(const std::string& prefix, const std::string& fmt, Arguments&&... args) {
        LOG_WITH_PREFIX(std::cout, prefix, fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGE(const std::string& fmt, Arguments&&... args) {
        auto elc = prepare_error_log();
        LOG(std::cerr, fmt, std::forward<Arguments>(args)...);
        finish_error_log(elc);
    }

    template<typename... Arguments>
    void LOGE_WITH_PREFIX(const std::string& prefix, const std::string& fmt, Arguments&&... args) {
        auto elc = prepare_error_log();
        LOG_WITH_PREFIX(std::cerr, prefix, fmt, std::forward<Arguments>(args)...);
        finish_error_log(elc);
    }

    template<typename... Arguments>
    void LOGIx(const std::string& fmt, Arguments&&... args) {
    }

    template<typename... Arguments>
    void LOGEx(const std::string& fmt, Arguments&&... args) {
    }
}

#if WIN32
#define __FILENAME__ (strrchr("\\" __FILE__, '\\') + 1)
#else
#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGIP(...) LOGI_WITH_PREFIX((boost::format("%||(%||): ") % __FILENAME__ % __LINE__).str(), __VA_ARGS__)
#define LOGEP(...) LOGE_WITH_PREFIX((boost::format("%||(%||): ") % __FILENAME__ % __LINE__).str(), __VA_ARGS__)
