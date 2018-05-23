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
    void LOG(std::ostream& stream, const std::string& fmt, Arguments&&... args) {
        try {
            boost::format f(fmt);
            stream << awesome_printf_helper(f, std::forward<Arguments>(args)...) << std::endl;
        } catch (boost::io::bad_format_string& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "fmt: " << fmt << std::endl;
        }
    }

    template<typename... Arguments>
    void LOGI(const std::string& fmt, Arguments&&... args) {
        LOG(std::cout, fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGE(const std::string& fmt, Arguments&&... args) {
        LOG(std::cerr, fmt, std::forward<Arguments>(args)...);
    }

    template<typename... Arguments>
    void LOGIx(const std::string& fmt, Arguments&&... args) {
    }

    template<typename... Arguments>
    void LOGEx(const std::string& fmt, Arguments&&... args) {
    }
}
