#pragma once

#include "tcp_connection.hpp"

namespace ss {
    class tcp_server {
    public:
        tcp_server(boost::asio::io_service& io_service);

    private:
        void start_accept();

        void handle_accept(tcp_connection::pointer new_connection,
                           const boost::system::error_code& error);

        tcp::acceptor acceptor_;
    };
}
