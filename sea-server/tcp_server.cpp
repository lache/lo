#include "precompiled.hpp"
#include "tcp_server.hpp"

using namespace ss;

extern int g_production;

tcp_server::tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, g_production ? tcp::endpoint(tcp::v4(), 53200) : tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 53200)) {
    start_accept();
}

void tcp_server::start_accept() {
    tcp_connection::pointer new_connection =
        tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
                           boost::bind(&tcp_server::handle_accept,
                                       this,
                                       new_connection,
                                       boost::asio::placeholders::error));
}

void tcp_server::handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code & error) {
    if (!error) {
        new_connection->start();
        new_connection->start_read_request();
    }
    // aceept another client
    start_accept();
}
