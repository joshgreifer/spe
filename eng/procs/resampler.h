#pragma once
#include "../resample_impl.h"
#include "../quick_queue.h"
#include "../scheduler.h"
#include "../processor.h"
#include <queue>

namespace sel {
	namespace eng {
		namespace proc {
			// resampler
			template<class traits=eng_traits<>, size_t OutputFs= traits::input_fs, size_t InW = traits::input_frame_size, size_t OutW=InW>class resampler :
				public semaphore, virtual public creatable<resampler<traits>>
			{
				using fifo = quick_queue<samp_t>;
				std::unique_ptr<fifo> pfifo_ = 0;

				// At init time, this is set by the output processor
				schedule *output_context = nullptr;

				struct resampler_in_proc_t : Processor1A0<InW>
				{
					std::unique_ptr<resampler_impl> pimpl_ = 0;
					resampler *owner;

					explicit resampler_in_proc_t(resampler *m) : owner(m) {}

					// Set up filters and create fifo big enough to buffer the resampled output 
					void init(schedule *context) final
					{
						if (context->trigger() == owner)
							throw eng_ex("Resampler input can't triggered by the resampler itself.");
						size_t input_fs = context->expected_rate().n();
						if (!input_fs)
							throw eng_ex("Can't create resampler, because its input sample rate numerator is unspecified.");

						size_t fifo_size;
						if (input_fs != OutputFs) {
							pimpl_ = std::move(std::make_unique<resampler_impl>(InW, input_fs, OutputFs));
							//fifo_size = 2 * sel::lcm(InW, OutW);

							fifo_size = 2 * pimpl_->output_frame_size();
						} else { // no resampling, just buffer (i.e. become a mux/demux)
							fifo_size = sel::lcm(InW, OutW);
						}
						owner->pfifo_ = std::move(std::make_unique<fifo>(fifo_size));
						owner->set_rate(OutputFs, OutW);
					}

					void process() final 
					{
						if (pimpl_) {
							pimpl_->resample(this->in, owner->pfifo_->acquirewrite());
							owner->pfifo_->endwrite(pimpl_->output_frame_size());

						} else {
							owner->pfifo_->atomicwrite(this->in, InW);
						}

						const  size_t semaphore_count = owner->pfifo_->get_avail() / OutW;
						if (semaphore_count)
							owner->output_context->invoke(semaphore_count);

					}
				} input_;

				struct resampler_out_proc_t : Processor01A<OutW>
				{
					resampler *owner;
					explicit resampler_out_proc_t(resampler *m) : owner(m) {}

					void init(schedule *context) final 
					{
						if (context->trigger() != owner)
							throw eng_ex("Resampler output must be triggered by the resampler itself.");
						owner->output_context = context;
					}

					void process() final
					{
						owner->pfifo_->atomicread_into(this->oport);
					}

				} output_;


			public:

				constexpr auto internal_window_size() const { return pfifo_->size(); }

//				virtual const std::string type() const override { return "resampler"; }

				resampler_in_proc_t & input() { return input_; }
				resampler_out_proc_t & output() { return output_; }

				// default constuctor needed for factory creation
				explicit resampler() : semaphore(0, rate_t(OutputFs, 1)), input_(this), output_(this)
				{
					
				}
				
				explicit resampler(params& params): resampler()
				{
				}

			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "resampler_ut.h"
#endif

