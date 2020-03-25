#pragma once
#include "../eng_traits.h"
#include "../processor.h"
#define _USE_MATH_DEFINES
#include <math.h>
namespace sel {
	namespace eng {
		namespace proc {


			namespace wintype {

				template<typename traits>struct KAISER
				{

					template<size_t Order=traits::input_frame_size>static double kaiser(size_t idx)
					{
						static auto initialized = false;
						static std::array<double, Order>coeffs = { -1 };
						if (!initialized) {
							constexpr double bta = traits::kaiser_beta;

							double bes = fabs(boost::math::cyl_bessel_i(0, bta));
							constexpr int odd = Order % 2;
							constexpr double xind = (Order - 1) * (Order - 1);
							constexpr size_t n = (Order + 1) / 2;

							std::array<double, n> xi;
							std::array<double, n> w;

							for (size_t i = 0; i < n; ++i) {
								double val = i + 0.5 * (1 - odd);
								xi[i] = 4.0 * val * val;
								w[i] = boost::math::cyl_bessel_i(0, bta * sqrt(1 - xi[i] / xind)) / bes;

							}

							size_t j = 0;
							// NOTE: i must be int, due to weird test 'i >= odd'
							for (int i = n - 1; i >= odd; --i)
								coeffs[j++] = fabs(w[i]);
							for (size_t i = 0; i < n; ++i)
								coeffs[j++] = fabs(w[i]);

							assert(j == Order);
							initialized = true;
						}
						return coeffs[idx];
					}

					template<size_t Winsize=traits::input_frame_size>static void process_buffer(const samp_t *in, samp_t *out)
					{

						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] * kaiser(i);

					}
					static const char *name() { return  "kaiser_window"; }

				};
				template<typename traits>struct HAMMING {
					template<size_t Winsize=traits::input_frame_size>static void process_buffer(const samp_t *in, samp_t *out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] *  0.54 - 0.46 * cos((2.0 * M_PI * i) / (Winsize - 1));

					}
					static const char *name() { return "hamming_window"; }

				};
				template<typename traits>struct HANN {
					template<size_t Winsize=traits::input_frame_size>static void process_buffer(const samp_t *in, samp_t *out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] * 0.5 - 0.5 * cos((2.0 * M_PI * i) / (Winsize - 1));

					}
					static const char *name() { return "hann_window"; }

				};
				template<typename traits>struct RECTANGULAR {
					template<size_t Winsize=traits::input_frame_size>static void process_buffer(const samp_t *in, samp_t *out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i];

					}
					static const char *name() { return "rectangular_window"; }

				};
			}

			/*
			Window
			*/
			template<typename traits, typename wintype, size_t input_sz, bool=(traits::overlap > 0)> class window_t;

			template<typename traits, typename wintype, size_t input_sz> class window_t<traits, wintype, input_sz, false> :
				public  Processor1A1B<traits::input_frame_size, traits::input_frame_size>, virtual public creatable<window_t<traits, wintype, input_sz>>
			{
			public:
				/*
				|
				*/

				virtual const std::string type() const override { return wintype::name(); }

				void process() final 
				{
					wintype::process_buffer(this->in, this->out);
				}

				void init(schedule *context) final {

				}

				window_t() {}
				window_t(params& params) {}


			};
			template<typename traits, typename wintype, size_t input_sz> class window_t<traits, wintype, input_sz, true> :
				public connectable_t<window_t<traits, wintype, input_sz>>, public  semaphore, virtual public creatable<window_t<traits, wintype, input_sz>>
			{
				static constexpr size_t output_sz = traits::input_frame_size;
				static_assert(traits::overlap < output_sz, "Window overlap must be less than window size.");
			public:
				using fifo = quick_queue<samp_t, output_sz>;
				fifo fifo_;
				// At init time, this is set by the output processor
				schedule *output_context = nullptr;

				struct in_proc_t :  Processor1A0<input_sz>
				{
					using input_buf = quick_queue<samp_t, 2 * std::max(input_sz, output_sz)>;
					input_buf input_buf_;

					wintype impl_;
					window_t *owner;
					explicit in_proc_t(window_t *o) : owner(o) {}

					void process() final {
						input_buf_.atomicwrite(this->in, input_sz);
						while (input_buf_.get_avail() >= output_sz)
						{
							impl_.process_buffer(input_buf_.atomicread(output_sz-traits::overlap), owner->fifo_.acquirewrite());
							owner->fifo_.endwrite(output_sz);
							owner->output_context->invoke();
							
						}

					}

					void init(schedule *context) final {
						if (context->trigger() == owner)
							throw eng_ex("Window input can't triggered by the window itself.");

						owner->set_rate(context->expected_rate() * rate_t(output_sz,output_sz-traits::overlap));
					}

				} input_;

				struct out_proc_t : Processor01A<traits::input_frame_size>
				{
					window_t *owner;
					explicit out_proc_t(window_t *o) : owner(o) {}

					void init(schedule *context) final 
					{
						if (context->trigger() != owner)
							throw eng_ex("Window output must be triggered by the window itself.");
						owner->output_context = context;
					}

					void process() final
					{
						owner->fifo_.atomicread_into(this->oport);
					}
					
				} output_;

				virtual const std::string type() const override { return wintype::name(); }

				in_proc_t & input() { return input_; }
				out_proc_t & output() { return output_; }

				template<class Impl>void ConnectTo(connectable_t<Impl>&to)
				{
					output_.ConnectTo(to);
				}

				template<class Impl>void ConnectFrom(connectable_t<Impl>&from)
				{
					input_.ConnectFrom(from);
				}

				

				explicit window_t() : semaphore(0, 0), input_(this), output_(this) {}

				explicit window_t(params& params) : window_t() {}


			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "window_ut.h"
#endif
