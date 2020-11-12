#pragma once
#include "../quick_queue.h"
#include "../processor.h"
#include "../input_stream.h"

namespace sel {
	namespace eng6 {
		namespace proc {

			template<size_t OUTW>class data_source : public Source<OUTW>, public semaphore {
			public:
//				virtual const std::string type() const override { return "data_source"; }
				data_source(rate_t expected_rate = rate_t()) : semaphore(0, expected_rate) {}
				void enable(bool enabled = true) const { semaphore::enabled = enabled;  }
				void invoke(size_t repeat_count = 1) { schedule(this, *this).invoke(repeat_count); }
			};
			/*
		Simple block-based input source.

		Its semaphore is initially 0 (not set).

		The underlying stream is responsible for raising the semaphore, which it does on successful connection, and after each read.
		*/

		template<class number_t, size_t OUTW, typename = typename std::enable_if<std::is_arithmetic<number_t>::value, number_t>::type >
	class scalar_stream_reader : public data_source<OUTW>, public stream_reader
	{
		// internal buffer size must be at least ( size of output  port X sizeof(number_t) )
		static constexpr size_t INTERNAL_BUFFER_SIZE_BYTES = std::max<size_t>(0x10000, OUTW * sizeof(number_t));

		using stream_ref_t = input_stream& ;
		using stream_ptr_t = input_stream *;
		stream_ptr_t istream_;
		size_t nitems_read;
		quick_queue<byte, INTERNAL_BUFFER_SIZE_BYTES, true> ibuf_;

	public:
//		virtual const std::string type() const override { return "number_stream_reader"; }

		scalar_stream_reader(stream_ptr_t stream, rate_t expected_rate) : data_source<OUTW>(expected_rate)
		{
			set_stream(stream);
		}

		void set_stream(stream_ptr_t stream)
		{
			nitems_read = 0;
			this->istream_ = stream;
			if (stream) 
				stream->reader = this;
			
		}

		void on_stream_connected(stream_ref_t s) final
		{

			// start the read pump
			s.beginread(ibuf_.acquirewrite(), ibuf_.put_avail());
		}

		void on_stream_data_recv_complete(stream_ref_t s, const size_t nbytes_read) final
		{
			/// BEGIN TRACE
			static size_t totbytes_read = 0;
			if (!totbytes_read) {
				std::cout << "Data stream started.\n";
			}
			totbytes_read += nbytes_read;
//			std::cout << "(on_data) " << nitems << " items read in on_stream_data_recv_complete().\n";
			/// END TRACE
//			std::cout << "(on_data) ibuf avail: " << ibuf_.get_avail() << "\n";

			ibuf_.endwrite(nbytes_read);

			size_t num_buffers_avail = ibuf_.get_avail() / (OUTW * sizeof(number_t));
			if (num_buffers_avail)
				this->raise(num_buffers_avail);
			else { 
				if (nbytes_read == 0) {
					const auto pad_value = (number_t)0;
					for (size_t i = 0; i < OUTW * sizeof(number_t); ++i)
						ibuf_.put(pad_value);
					this->raise();
				} else //  keep reading until num_buffers_avail >= 1
					istream_->beginread(ibuf_.acquirewrite(), ibuf_.put_avail());
			} 


//			std::cout << "(on_data) raised: " << num_buffers_avail << "\n";

		}

		void term(schedule *context) final {
			istream_->disconnect();
		}

		void process(void) override
		{
//			std::cout << "(process) ibuf avail: " << ibuf_.get_avail() << "\n";

#define DEBUG_STREAM_READER
#if (defined(DEBUG_STREAM_READER) && !defined(NDEBUG))

			const size_t num_buffers_avail = ibuf_.get_avail() / (OUTW * sizeof(number_t));

			assert(num_buffers_avail);
			// semaphore count should match num_buffers_avail before semaphore was acquired (i.e. when its value was 1 greater than it is now)
			assert(num_buffers_avail-1 == this->count());

#endif
			// dequeue onto output port - note, doesn't handle endianness.
			// to do so, need to modify below line, can't just cast pointer
			// TODO: Handle endianness
			number_t *p = (number_t *)ibuf_.atomicread(OUTW * sizeof(number_t));
			samp_t *q = this->out;
			for (size_t i = 0; i < OUTW; ++i)
				*q++ = static_cast<samp_t>(p[i]);

		
//			ibuf_.atomicread(OUTW * sizeof(number_t));


			
			// trigger another read if no reads pending and no more buffers avail
			if (this->count() == 0)
				if (byte *w = ibuf_.acquirewrite())
					istream_->beginread(w, ibuf_.put_avail());
		}
	};

	template <class number_t, size_t INW, size_t OUTPUT_BUFFER_SIZE = 16384 / sizeof(number_t), typename = typename std
	          ::enable_if<std::is_arithmetic<number_t>::value, number_t>::type>
	class number_stream_writer : public Processor1A0<INW>, public stream_writer
	{
		// internal buffer size must be at least ( size of output  port X sizeof(number_t) )
		static constexpr size_t INTERNAL_BUFFER_SIZE_BYTES = std::max<size_t>(0x10000, INW * sizeof(number_t));
		static constexpr size_t OUTPUT_BUFFER_SIZE_BYTES = std::max<size_t>(OUTPUT_BUFFER_SIZE, 1) * sizeof(number_t);

		using stream_ref_t = output_stream &;
		using stream_ptr_t = output_stream *;
		stream_ptr_t ostream_;
		quick_queue<byte, INTERNAL_BUFFER_SIZE_BYTES, true> obuf_;
		bool stream_connected_;

	public:
//		virtual const std::string type() const override { return "number_stream_writer"; }
		number_stream_writer(stream_ptr_t stream) : stream_connected_(false)
		{
			set_stream(stream);
		}

		void set_stream(stream_ptr_t stream)
		{
			this->ostream_ = stream;
			if (stream)
				stream->writer = this;

		}

		void on_stream_connected(stream_ref_t s) final
		{
			stream_connected_ = true;
		}

		void on_stream_data_send_complete(stream_ref_t s, const size_t nbytes_transferred) final
		{
			obuf_.endread(nbytes_transferred);
		}

		void term(schedule *context) final {
			// flush buffer before term
			if (stream_connected_) {
				auto nbytes_to_write = obuf_.get_avail();
				if (nbytes_to_write)
					if (const byte *r = obuf_.acquireread())
						ostream_->write(r, nbytes_to_write, false);

			}

			ostream_->disconnect();
			stream_connected_ = false;
		}

		void process(void) override
		{
			//			std::cout << "(process) ibuf avail: " << ibuf_.get_avail() << "\n";

			if (obuf_.put_avail() >= INW * sizeof(number_t)) {

				number_t obuf_as_number_t[INW];

				for (size_t i = 0; i < INW; ++i)
					obuf_as_number_t[i] = static_cast<number_t>(this->in[i]);
				

				obuf_.atomicwrite(reinterpret_cast<byte *>(obuf_as_number_t), INW * sizeof(number_t));
			}
					

			// trigger a write if no writes pending and no more buffers avail
			if (stream_connected_) { // on disconnect, stream_connected is set false, and we flush the buffers synchronously
				size_t nbytes_avail = obuf_.get_avail();
				if (nbytes_avail >= OUTPUT_BUFFER_SIZE_BYTES) // force buffered output
					if (const byte *w = obuf_.acquireread())
						ostream_->write(w, nbytes_avail, false);

			}
			

		}
	};

		} // proc
	} // eng
} //sel

