#pragma once
namespace ss {
    using boost::asio::ip::tcp;

    class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
    public:
        typedef std::shared_ptr<tcp_connection> pointer;

        static pointer create(boost::asio::io_service& io_service);

        tcp::socket& socket();

        void start();
        void start_read_request();

    private:
        tcp_connection(boost::asio::io_service& io_service);
        void handle_write(const boost::system::error_code& /*error*/,
                          size_t /*bytes_transferred*/);
        void do_read_command();
        void do_read_float1();
        void do_read_float2();
        tcp::socket socket_;
        std::string message_;
        std::array<char, 1024> recv_buffer_;
    };
}
