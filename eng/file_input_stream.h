#pragma once
#include <fcntl.h> 
#ifdef _WIN32
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#define tell(fd) lseek(fd, 0, SEEK_CUR)
#define O_BINARY 0 /* Not available in Linux */
#endif
#include "input_stream.h"
#include "msg_and_error.h"
#include "scheduler.h"
#include "quick_queue.h"

// read() vs _read() etc
#pragma warning(disable : 4996)
namespace sel {
	namespace eng {
		class file_input_stream : public input_stream
		{
			using file_descriptor = int;


			using std_ec = std::error_code;

			file_descriptor fd_;

			bool is_eof = false;

		public:

			void beginread(byte* buf, size_t bytes_requested) final
			{
				// Don't dispatch any co-routines if eof
				if (is_eof) {
					// Call error handler,  which may want to abort on eof
					std::error_code ec = eng_errc::input_stream_eof;
					this->on_error(ec);
				}
				int bytes_transferred = ::read(fd_, buf, (unsigned int)bytes_requested);

				if (bytes_transferred < 0) {
					std::error_code ec = std::system_error(errno, std::system_category()).code();
					this->on_error(ec);

				}
				else {
					if (bytes_transferred == 0) { // EOF
						this->is_eof = true;

					}

					// Simulate async read by dispatching, rather than calling, read completion co-routine.
					// Note, we do this even if bytes_transferred == 0.
					// If a reader subsequently triggers another beginread(),
					// this->is_eof will be set, and no more read handlers will be dispatched.
#ifdef _WIN32
					scheduler::get().queue_work_item([this, bytes_transferred] {
#endif
						this->endread(bytes_transferred);
#ifdef _WIN32
						});
#endif
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
				fd_ = fd;
				auto pos = tell(fd_);
				this->on_connected();


				return  std_ec(errno, std::system_category());
			}

			std_ec disconnect() final {
				close(fd_);
				fd_ = -1;
				return std_ec(errno, std::system_category());
			}

		};
	} // eng
} // sel
