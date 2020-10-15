#pragma once
#include <system_error>
#include "processor.h"
#include "uri.h"

namespace sel {
	namespace eng {
		typedef unsigned char byte;

		class bytestream;
		class input_stream;
		class output_stream;


		// One and only one reader for each stream
		class stream_reader
		{
		public:
			virtual void on_stream_connected(input_stream& s) = 0;
			virtual void on_stream_data_recv_complete(input_stream& s, const size_t nbytes) = 0;

			virtual void on_error(bytestream& s, std::error_code& ec) throw(...) {
				throw ec;
				// std::cerr << "Error reading stream: " << ec.message() << std::endl;
			}

		};

		class stream_writer
		{
		public:
			virtual void on_stream_connected(output_stream& s) = 0;
			virtual void on_stream_data_send_complete(output_stream& s, const size_t nbytes) = 0;


			virtual void on_error(bytestream& s, std::error_code& ec) {
				std::cerr << "Error writing stream: " << ec.message() << std::endl;
			}

		};

		class bytestream
		{
		public:
			virtual ~bytestream() = default;


			using std_ec = std::error_code;


			virtual std_ec connect(const uri& uri, func on_connected ) = 0;

			virtual std_ec disconnect() = 0;

			// error status
			const std_ec& status() const { return last_error; }


		protected:

			std_ec last_error;  // last os error

								  // Handle, and possibly clear, the error status

			virtual void on_connected() = 0;



		};


		class input_stream : public virtual bytestream
		{

		public:
			stream_reader *reader;

			input_stream() : reader(nullptr) {}
			// read data (typically from internal buffer(s) ).   Will raise error if unable to fill dest completely.
			// read() is called from an 'input_source' processor's process() routine,  where 'dest' is one of
			// the processor's output ports.
			//
			// Typically, a read() implementation will call fill_buffers() before it completes, to maintain its internal buffers.
			// If the read fails, it returns nullptr.
			virtual void beginread(byte *buf, size_t bytes_requested) = 0;


		protected:

			// Alert reader when connected
			virtual void on_connected()  {
				if (reader)
					reader->on_stream_connected(*this);
			}

			virtual void endread(const size_t nbytes_read)   {
				if (reader)
					reader->on_stream_data_recv_complete(*this, nbytes_read);
			}

			virtual void on_error(std::error_code& ec) {
				if (reader)
					reader->on_error(*this, ec);
			}

		};

		class output_stream : public virtual bytestream
		{

		public:
			stream_writer *writer;

			output_stream() : writer(nullptr) {}
	
			virtual void write( const byte *buffer, size_t nbytes_to_send, bool async ) = 0;


		protected:

			// Alert reader when connected
			virtual void on_connected() {
				if (writer)
					writer->on_stream_connected(*this);
			}

			virtual void endwrite(const size_t nbytes) {
				if (writer)
					writer->on_stream_data_send_complete(*this, nbytes);
			}

			virtual void on_error(std::error_code& ec) {
				if (writer)
					writer->on_error(*this, ec);
			}

		};

		class io_stream : public input_stream, public output_stream
		{
		protected:

			virtual void on_error(std::error_code& ec) {
				
				input_stream::on_error(ec);
				output_stream::on_error(ec);
			}

			virtual void on_connected() {
				input_stream::on_connected();
				output_stream::on_connected();
			}

		};
	}
}
