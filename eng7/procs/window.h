#pragma once
#include "../../eng/eng_traits.h"
#include "../new_processor.h"
#include "../../eng/quick_queue.h"
#define _USE_MATH_DEFINES

#include <math.h>
#include <boost/math/special_functions/bessel.hpp>

namespace sel {
	namespace eng7 {
		namespace proc {


			namespace wintype {

				template<typename traits>struct KAISER
				{

					template<size_t Order = traits::input_frame_size>static double kaiser(size_t idx)
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

					template<size_t Winsize = traits::input_frame_size>static void process_buffer(const samp_t* in, samp_t* out)
					{

						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] * kaiser(i);

					}
					static const char* name() { return  "kaiser_window"; }

				};
				template<typename traits>struct HAMMING {
					template<size_t Winsize = traits::input_frame_size>static void process_buffer(const samp_t* in, samp_t* out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] * (0.54 - 0.46 * cos((2.0 * M_PI * i) / Winsize));

					}
					static const char* name() { return "hamming_window"; }

				};
				template<typename traits>struct HANN {
					template<size_t Winsize = traits::input_frame_size>static void process_buffer(const samp_t* in, samp_t* out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i] * (0.5 - 0.5 * cos((2.0 * M_PI * i) / Winsize));

					}
					static const char* name() { return "hann_window"; }

				};
				template<typename traits>struct RECTANGULAR {
					template<size_t Winsize = traits::input_frame_size>static void process_buffer(const samp_t* in, samp_t* out)
					{
						for (size_t i = 0; i < Winsize; ++i)
							out[i] = in[i];

					}
					static const char* name() { return "rectangular_window"; }

				};
			}

			/*
			Window
			*/
			template<typename traits, typename wintype, size_t input_sz, bool = (traits::overlap > 0)> class window_t;

			template<typename traits, typename wintype, size_t input_sz> class window_t<traits, wintype, input_sz, false> :
				public  stdproc<traits::input_frame_size, traits::input_frame_size>
			{
			public:
				/*
				|
				*/


				void process() final
				{
					wintype::process_buffer(this->in_v().data(), this->out().data());
				}

				void init(eng::schedule* context) final {

				}

				window_t() {}
				window_t(params& params) {}


			};

			template<typename traits, typename wintype, size_t input_sz> class window_t<traits, wintype, input_sz, true> :
		public processor,
		public eng::semaphore
			{
                std::array<samp_t, input_sz>* input_port;
                std::array<samp_t, input_sz>* output_port;


                auto& in() { return input_port; }
               const auto in_v() const { return *input_port; }
                auto& out() { return output_port; }
                auto out() const {return output_port; }

                template<class TO_PROC, size_t from_pin = 0, size_t to_pin = 0> void connect_to(TO_PROC& to) {
                    static_assert(TO_PROC::has_inputs, "Can't connect: 'to' Processor has no inputs.");
                    static_assert(has_outputs, "Can't connect: 'from' Processor has no outputs.");

                    to.template in<to_pin>() = &output_port;
                }

				static constexpr size_t output_sz = traits::input_frame_size;
				static_assert(traits::overlap < output_sz, "Window overlap must be less than window size.");
			public:
				using fifo = sel::quick_queue<samp_t, output_sz>;
				fifo fifo_;
				// At init time, this is set by the output processor
				eng::schedule* output_context = nullptr;

				struct in_proc_t : stdsink<input_sz>
				{
					using input_buf = sel::quick_queue<samp_t, 2 * ::std::max(input_sz, output_sz)>;
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

					void init(eng::schedule* context) final {
						if (context->trigger() == owner)
							throw eng_ex("Window input can't triggered by the window itself.");

						owner->set_rate(context->expected_rate() * rate_t(output_sz, output_sz - traits::overlap));
					}

				} input_;


				void init(eng::schedule* context) final
				{
					if (context->trigger() != this)
						throw eng_ex("Window output must be triggered by the window itself.");
					this->output_context = context;
				}

				void process() final
				{
					this->fifo_.atomicread_into(this->oport);
				}


				explicit window_t() : input_(this) {}

				explicit window_t(params& params) : window_t() {}


			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "window_ut.h"
#endif
