#pragma once

namespace ss {
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;
    using boost::asio::ip::udp;
    class sea;
    class sea_static;
    class seaport;
    class shipyard;
    class udp_server;
    class session;
    class city;
    class udp_admin_server {

    public:
        udp_admin_server(boost::asio::io_service& io_service,
                         std::shared_ptr<sea> sea,
                         std::shared_ptr<sea_static> sea_static,
                         std::shared_ptr<seaport> seaport,
                         std::shared_ptr<shipyard> shipyard,
                         std::shared_ptr<udp_server> udp_server,
                         std::shared_ptr<session> session,
                         std::shared_ptr<city> city);
        void send_recover_all_ships();
        void send_arrival(int ship_id);
        int process_command(const unsigned char* command, bool send_reply);
    private:
        void start_receive();

        // How to test handle_receive():
        // $ perl -e "print pack('ff',10.123,20.456)" > /dev/udp/127.0.0.1/3100

        void handle_receive(const boost::system::error_code& error,
                            std::size_t bytes_transferred);

        void handle_send(const boost::system::error_code& error,
                         std::size_t bytes_transferred);

        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        std::array<char, 1024> recv_buffer_;
        std::shared_ptr<sea> sea_;
        std::shared_ptr<sea_static> sea_static_;
        std::shared_ptr<seaport> seaport_;
        std::shared_ptr<shipyard> shipyard_;
        std::shared_ptr<city> city_;
        std::shared_ptr<udp_server> udp_server_;
        std::shared_ptr<session> session_;
        // sea-server -> web-server requests
        udp::resolver resolver_;
        udp::resolver::query web_server_query_;
        udp::endpoint web_server_endpoint_;
        udp::socket web_server_socket_;
        //boost::asio::io_service& io_service_;
    };
}
