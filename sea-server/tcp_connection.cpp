#include "precompiled.hpp"
#include "tcp_connection.hpp"

using namespace ss;

static std::string make_daytime_string() {
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

tcp_connection::pointer tcp_connection::create(boost::asio::io_service& io_service) {
    return pointer(new tcp_connection(io_service));
}

tcp::socket & tcp_connection::socket() {
    return socket_;
}

void tcp_connection::start() {
    message_ = make_daytime_string();

    boost::asio::async_write(socket_,
                             boost::asio::buffer(message_),
                             boost::bind(&tcp_connection::handle_write,
                                         shared_from_this(),
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
}

tcp_connection::tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service) {
}

void tcp_connection::handle_write(const boost::system::error_code & error, size_t bytes_written) {
    if (error) {
        LOGE("ERROR: %s", error);
    } else {
        LOGI("%d bytes written.", bytes_written);
    }
}

void tcp_connection::do_read_command() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(recv_buffer_.data(), 4),
                            [this, self](const boost::system::error_code & error, size_t bytes_read) {
        if (error) {
            LOGE("ERROR: %s", error);
            return;
        } else {
            LOGI("%1% bytes read. request code: %2%",
                 bytes_read,
                 *reinterpret_cast<int*>(recv_buffer_.data()));

            do_read_float1();
        }
    });
}

void tcp_connection::do_read_float1() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(recv_buffer_.data(), 4),
                            [this, self](const boost::system::error_code & error, size_t bytes_read) {
        if (error) {
            LOGE("ERROR: %s", error);
            return;
        } else {
            LOGI("%1% bytes read. float 1: %2%",
                 bytes_read,
                 *reinterpret_cast<float*>(recv_buffer_.data()));
            do_read_float2();
        }
    });

}

void tcp_connection::do_read_float2() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(recv_buffer_.data(), 4),
                            [this, self](const boost::system::error_code & error, size_t bytes_read) {
        if (error) {
            LOGE("ERROR: %s", error);
            return;
        } else {
            LOGI("%1% bytes read. float 2: %2%",
                 bytes_read,
                 *reinterpret_cast<float*>(recv_buffer_.data()));
        }
        // read another request
        start_read_request();
    });
}

void tcp_connection::start_read_request() {
    do_read_command();


}
