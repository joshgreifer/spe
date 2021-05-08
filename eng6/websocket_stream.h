#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "input_stream.h"
#include "msg_and_error.h"
#include "scheduler.h"
namespace sel {
	namespace eng6 {
		namespace ip = boost::asio::ip;
		using tcp = ip::tcp;               // from <boost/asio/ip/tcp.hpp>
		namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

		class websocket_stream : public io_stream
		{

		private:
			boost::asio::io_context& ioc_;
			tcp::resolver resolver_;
			tcp::acceptor acceptor_;
			tcp::socket socket_;
			websocket::stream<boost::beast::tcp_stream> ws_;
			std::string host_;
			std::string port_;  // *not* int

		public:

			websocket_stream() :
				ioc_(asio_scheduler::get().ioc()),
				resolver_(ioc_),
				acceptor_(ioc_),
				socket_(ioc_),
				ws_(std::move(socket_))
			{}

			using err_code = std::error_code;


			void write(const byte *buffer, size_t nbytes_to_send, bool async) final
			{
				ws_.binary(true);
				if (async) {
					ws_.async_write(boost::asio::buffer(buffer, nbytes_to_send), [this](boost::system::error_code ec, std::size_t bytes_transferred) {
						if (ec)
							throw eng_ex(ec.message());
						else
							this->endwrite(bytes_transferred);
					}
					);
				} else {
					
					try {
						size_t bytes_transferred = ws_.write(boost::asio::buffer(buffer, nbytes_to_send));
						this->endwrite(bytes_transferred);

					} catch (std::exception& ec) {
                        throw eng_ex(ec.what());
					}
				}
			}

			void beginread(byte *buf, size_t bytes_requested) final
			{
				if (bytes_requested && ws_.is_open()) {
					ws_.async_read_some(boost::asio::buffer(buf, bytes_requested), [this](boost::system::error_code ec, std::size_t bytes_transferred) {
					if (ec)
						throw eng_ex(ec.message());
					this->endread(bytes_transferred);
					});
				}
			}
		private:

            void do_accept(func on_connected) {
                acceptor_.async_accept(get_lowest_layer(ws_).socket(), [this, on_connected](boost::system::error_code ec) {
                    if (ec)
                        throw eng_ex(ec.message());
//					ws_ = std::move(f); // websocket::stream<tcp::socket>(socket_);

                    // Set suggested timeout settings for the websocket
                    ws_.set_option(
                            websocket::stream_base::timeout::suggested(
                                    boost::beast::role_type::server));
                    ws_.accept(ec);
                    if (ec)
                        throw eng_ex(ec.message());
                    on_connected();
                    // Tell reader to start the read pump
                    this->on_connected();
//                    accept(on_connected);

                });

			}
		public:
			err_code connect(const uri& uri, func on_connected) final {

				uri.assert_scheme(uri::WS);
				// ----------------------------------------------------------------------------------------------
				// parse uri:  format  hostname:port
				//
				// look for colon character which precedes port number
				std::string s = uri.path();
				size_t delim_pos = s.find(':');
				if (delim_pos == std::string::npos)
					throw eng_ex("No colon specified in websocket address.  Uri format is ws://hostname:port");

				host_ = s.substr(0, delim_pos);
				port_ = s.substr(delim_pos + 1);

				printf("Listening for websocket connections on %s:%s...\n", host_.c_str(), port_.c_str());

				auto results = resolver_.resolve(host_, port_);

				// https://www.boost.org/doc/libs/1_67_0/libs/beast/example/websocket/server/async/websocket_server_async.cpp

				//auto const address = ip::make_address(host_);
				//const auto address = results.begin();
				// const auto port = static_cast<unsigned short>(std::atoi(port_.c_str()));

				//auto endpoint = tcp::endpoint{ address, port };
				auto endpoint = results->endpoint();

				boost::system::error_code ec;

 				acceptor_.open(endpoint.protocol(), ec);
				if (ec)
					throw eng_ex(ec.message());

                // Allow address reuse
                acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
                if (ec)
                    throw eng_ex(ec.message());

                acceptor_.bind(endpoint, ec);
				if (ec)
					throw eng_ex(ec.message());

				acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
				if (ec)
					throw eng_ex(ec.message());

                do_accept(on_connected);


#if NOT_A_SERVER
				auto results = resolver_.resolve(host_, port_);

				boost::asio::async_connect(ws_.next_layer(), results.begin(), results.end(), [this, uri](boost::system::error_code ec,
					tcp::resolver::iterator iterator) {
					if (ec) {
						printf("Attempt to connect to websocket on %s:%s failed (%s). Retrying...\n", host_.c_str(), port_.c_str(), ec.message().c_str());
						ioc_.queue_work_item([this, uri] { this->last_error = connect(uri); });
						// throw eng_ex(ec.message());
					}
					else {
						printf("Connected to websocket on %s:%s, performing handshake\n", host_.c_str(), port_.c_str());
						ws_.async_handshake(host_, "/data", [this](boost::system::error_code ec) {
							if (ec)
								throw eng_ex(ec.message());
							printf("Websocket handshake complete.\n");
						});

					}
				});
#endif
				return this->status();
			}

			err_code disconnect() final {
				//auto cc = websocket::close_code::none;
				//if (ws_.is_open())
				//	try { ws_.close(cc); } catch (...) {}
				return this->status();
			}

		};
}  // eng
} // sel
