#pragma once
#include "../new_processor.h"
#include "../../eng6/scheduler.h"
namespace sel
{
	namespace eng7
	{
		template<size_t input_sz, size_t output_t> class rate_changer_t :
			public stdproc<input_sz, output_t>
		{
			static constexpr size_t output_sz = traits::input_frame_size;
			static_assert(traits::overlap < output_sz, "Window overlap must be less than window size.");
		public:
			using fifo = quick_queue<samp_t, output_sz>;
			fifo fifo_;
			// At init time, this is set by the output processor
			schedule* output_context = nullptr;

			struct in_proc_t : stdsink<input_sz>
			{
				using input_buf = quick_queue<samp_t, 2 * std::max(input_sz, output_sz)>;
				input_buf input_buf_;

				wintype impl_;
				window_t* owner;
				explicit in_proc_t(window_t* o) : owner(o) {}

				void process() final {
					input_buf_.atomicwrite(this->in, input_sz);
					while (input_buf_.get_avail() >= output_sz)
					{
						impl_.process_buffer(input_buf_.atomicread(output_sz - traits::overlap), owner->fifo_.acquirewrite());
						owner->fifo_.endwrite(output_sz);
						owner->output_context->invoke();

					}

				}

				void init(schedule* context) final {
					if (context->trigger() == owner)
						throw eng_ex("Window input can't triggered by the window itself.");

					owner->set_rate(context->expected_rate() * rate_t(output_sz, output_sz - traits::overlap));
				}

			} input_;


			void init(schedule* context) final
			{
				if (context->trigger() != this)
					throw eng_ex("Window output must be triggered by the window itself.");
				this->output_context = context;
			}

			void process() final
			{
				this->fifo_.atomicread_into(this->oport);
			}



			virtual const std::string type() const override { return wintype::name(); }


			ConnectableProcessor& input_proc() final { return input_; }


			template<class Impl>void ConnectFrom(connectable_t<Impl>& from)
			{
				input_.ConnectFrom(from);
			}



			explicit window_t() : input_(this) {}

			explicit window_t(params& params) : window_t() {}


		};
	}
}

