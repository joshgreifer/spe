#pragma once

#include "../resample_impl.h"
#include "../quick_queue.h"
#include "../scheduler.h"
#include "../processor.h"
#include <queue>
#if 0
namespace sel {
	namespace eng {
		namespace proc {
			// rate changer
			template<
				typename Impl_,
				class traits=eng_traits<>, 
			size_t InW = traits::window_size, 
			size_t OutW=InW,
			size_t FifoSize
			
			>struct rate_changer :
				public semaphore
			{
			protected:
				schedule *output_context_ = nullptr;

				quick_queue<samp_t, FifoSize, true>fifo_;

				struct in_proc_t : Processor1A0<InW>
				{
					std::unique_ptr<Impl_> pimpl_;
					rate_changer *owner;

					quick_queue<samp_t, WindowSize * 2, true>input_window_;


					bool rates_match = false;

					explicit in_proc_t(rate_changer *m) : owner(m) {}

					void init(schedule *context) final
					{
						if (context->trigger == owner)
							throw eng_ex("Rate changer input can't triggered by the rate changer itself.");
						size_t input_fs = context->expected_rate();
						{
							pimpl_ = std::move(std::make_unique<Impl_>(WindowSize, input_fs, OutputFs));

						}

					}

					void process() final 
					{
						//owner->fifo_.atomicwrite(this->in, Inw);
						//size_t semaphore_count = owner->fifo_.get_avail() / Outw;
						//if (semaphore_count) {
						//	// now output port is ready
						//	owner->output_.schedule_->invoke(semaphore_count);
						//}
						if (rates_match) {
							owner->fifo_.atomicwrite_from<InW>(this->in);
							owner->output_.context->invoke();

						} else {
							input_window_.atomicwrite_from<InW>(this->in);
							if (input_window_.get_avail() >= WindowSize) {

								pimpl_->resample(input_window_.atomicread(WindowSize), owner->fifo_.acquirewrite());
								owner->fifo_.endwrite( pimpl_->output_size());

							}
							const  size_t semaphore_count = owner->fifo_.get_avail() / OutW;
							if (semaphore_count)
								owner->output_.context->invoke(semaphore_count);

						}
					}
				} input_;

				struct resampler_out_proc_t : Processor01A<OutW>
				{
					schedule *context = nullptr;

					resampler *owner;
					explicit resampler_out_proc_t(resampler *m) : owner(m) {}

					void init(schedule *context) final 
					{
						if (context->trigger != owner)
							throw eng_ex("Resampler output must be triggered by the resampler itself.");
						this->context = context;
					}

					void process() final
					{
						owner->fifo_.atomicread_into(this->oport);
					}

				} output_;


			public:

				static constexpr auto internal_window_size() { return WindowSize; }

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
#include "rate_changer_ut.h"
#endif

#endif // 0