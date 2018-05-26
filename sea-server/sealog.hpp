#pragma once
namespace ss {
    static std::string awesome_printf_helper(boost::format& f) {
        return boost::str(f);
    }

    template<class T, class... Args>
    std::string awesome_printf_helper(boost::format& f, T&& t, Args&&... args) {
        try {
            return awesome_printf_helper(f % std::forward<T>(t), std::forward<Args>(args)...);
        } catch (boost::io::bad_format_string& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "format: " << f << ", T: " << t << std::endl;
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
            std::cerr << e.what() << std::endl;
            std::cerr << "fmt: " << fmt << std::endl;
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
        LOG(std::cerr, fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGE_WITH_PREFIX(const std::string& prefix, const std::string& fmt, Arguments&&... args) {
        LOG_WITH_PREFIX(std::cerr, prefix, fmt, std::forward<Arguments>(args)...);
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
