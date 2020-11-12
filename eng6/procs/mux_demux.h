#pragma once
#include "../scheduler.h"
#include "../processor.h"
#include "../quick_queue.h"
#include "../event.h"

namespace sel {
	namespace eng6 {
		namespace proc {
			struct mux_demux_impl {};
			/*
			Signal multiplexer
			*/
			template<size_t Inw, size_t Outw>class mux_demux : public semaphore
			{
				quick_queue<samp_t, sel::lcm(Inw,Outw), true>fifo_;

				struct mux_in_proc_t : Processor1A0<Inw>
				{
					std::unique_ptr<mux_demux_impl> pimpl_;

					mux_demux *owner;
					mux_in_proc_t(mux_demux *m) : owner(m) {}

					void init(schedule *context) final
					{
						if (context->trigger() == owner)
							throw eng_ex("Mux/Demux input can't triggered by the mux_demux itself.");
						owner->set_rate(context->expected_rate() / Outw);
						pimpl_ = std::move(std::make_unique<mux_demux_impl>());

					}

					void process() final 
					{
						owner->fifo_.atomicwrite(this->in, Inw);
						size_t semaphore_count = owner->fifo_.get_avail() / Outw;
						if (semaphore_count) {
							// now output port is ready
							owner->output_.context->invoke(semaphore_count);
							//owner->raise(semaphore_count);
						}

					}
				} input_;

				struct mux_out_proc_t : Processor01A<Outw>
				{
					schedule *context = nullptr;

					mux_demux *owner;
					mux_out_proc_t(mux_demux *m) : owner(m) {}

					void init(schedule *context) final 
					{
						if (context->trigger() != owner)
							throw eng_ex("Mux/Demux output must be triggered by the mux_demux itself.");
						this->context = context;
					}

					void process() final
					{
						owner->fifo_.atomicread_into(this->oport);
					}

				} output_;

			public:
				mux_in_proc_t & input() { return input_; }
				mux_out_proc_t & output() { return output_; }
				mux_demux() : input_(this), output_(this) {}
				mux_demux(params& params): input_(this), output_(this)
				{
				}
			};



		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "mux_demux_ut.h"
#endif