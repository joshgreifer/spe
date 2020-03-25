#pragma once
#include <fcntl.h> 
#ifdef _WIN32
#include <io.h>
#endif
#include "input_stream.h"
#include "msg_and_error.h"
#include "scheduler.h"
#include "quick_queue.h"
// read() vs _read() etc
#pragma warning(disable : 4996)
namespace sel{
	namespace eng {
	class file_input_stream : public input_stream
	{
		using file_descriptor = int;


		using std_ec = std::error_code;

		file_descriptor fd_;

	public:

		void beginread(byte *buf, size_t bytes_requested) final
		{
			int bytes_transferred = ::read(fd_, buf, (unsigned int)bytes_requested);

			if (bytes_transferred < 0) {
				std::error_code ec = std::system_error(errno, std::system_category()).code();
				this->on_error(ec);

			}
			else if (bytes_transferred == 0) { // EOF
				std::error_code ec = eng_errc::input_stream_eof;
				this->on_error(ec);

			}
			else {
				// simulate async read by dispatching call back
				scheduler::get().queue_work_item([this, bytes_transferred] {
					this->endread(bytes_transferred);
				});
			}

		}

		std_ec connect(const uri& uri, func on_connected) final
		{
			uri.assert_scheme(uri::FILE);

			fd_ = ::open(uri.path(), O_RDONLY);
			if (fd_ < 0) {
				throw eng_ex(format_message("%s: %s", uri.path(), os_err_message<2048U>(errno).data()));
			}

			this->on_connected();


			return std_ec(errno, std::system_category());
		};

		std_ec attach(file_descriptor fd)
		{
			fd_ = ::dup(fd);
			this->on_connected();


			return  std_ec(errno, std::system_category());
		}

		std_ec disconnect() final {
			::close(fd_);
			fd_ = 0;
			return std_ec(errno, std::system_category());
		}

	};
} // eng
} // sel
