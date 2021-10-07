#pragma once
#include "../resample_impl.h"
#include "../quick_queue.h"
#include "../scheduler.h"
#include "../processor.h"
#include <queue>

namespace sel {
	namespace eng6 {
		namespace proc {
			// resampler
			template<class traits=eng_traits<>, size_t OutputFs= traits::input_fs, size_t InW = traits::input_frame_size, size_t OutW=InW>class resampler :
				public data_source<OutW>, virtual public creatable<resampler<traits>>
			{
				using fifo = quick_queue<samp_t>;
				std::unique_ptr<fifo> pfifo_ = nullptr;

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


                template<class Impl>void ConnectFrom(connectable_t<Impl>& from)
                {
                    input_.ConnectFrom(from);
                }

				void init(schedule *context) final 
				{
					if (context->trigger() != this)
						throw eng_ex("Resampler output must be triggered by the resampler itself.");
					this->output_context = context;
				}

				void process() final
				{
					this->pfifo_->atomicread_into(this->oport);
				}

	


			public:

				constexpr auto internal_window_size() const { return pfifo_->size(); }

//				virtual const std::string type() const override { return "resampler"; }


				ConnectableProcessor& input_proc() final { return input_; }
				
				// default constuctor needed for factory creation
				explicit resampler() : data_source<OutW>(rate_t(OutputFs, 1)), input_(this)
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
#include "samples.h"
#include "../unit_test.h"

SEL_UNIT_TEST(resampler)

        sel::eng6::scheduler scheduler = {};

        struct ut_traits
        {

            static constexpr size_t input_fs = 20000;
            static constexpr size_t output_fs = 10000;
            static constexpr size_t input_frame_size = 10;
            static constexpr size_t output_frame_size = 10;
            static constexpr size_t seconds_to_run = 1;
            static constexpr size_t iters_to_run = static_cast<size_t>(seconds_to_run * input_fs / static_cast<double>(input_frame_size));

        };
        using resampler = sel::eng6::proc::resampler <ut_traits,
                ut_traits::output_fs,
                ut_traits::input_frame_size,
                ut_traits::output_frame_size>;

        struct sig_gen_ramp : sel::eng6::proc::data_source<ut_traits::input_frame_size>
        {

            size_t c = 0;

            sig_gen_ramp() : sel::eng6::proc::data_source<ut_traits::input_frame_size>(rate_t(ut_traits::input_fs, 1)) {}

            void process() final
            {
                for (size_t i = 0; i < ut_traits::input_frame_size; ++i)
                    out[i] = static_cast<samp_t>(c++);
                std::cout << std::endl;
                for (size_t i = 0; i < 10; ++i)
                    std::cout << this->out_as_array(0)[i] << ' ';

            }
        };

        struct sig_gen_sine : sel::eng6::proc::data_source<ut_traits::input_frame_size>
        {

            samp_t t = 0.0;

            sig_gen_sine() : sel::eng6::proc::data_source<ut_traits::input_frame_size>(rate_t(ut_traits::input_fs, 1)) {}

            void process() final
            {
                for (size_t i = 0; i < ut_traits::input_frame_size; ++i)
                    out[i] = sin(t += 1.0/ut_traits::input_fs);
                std::cout << std::endl;
                for (size_t i = 0; i < 10; ++i)
                    std::cout << this->out_as_array(0)[i] << ' ';

            }
        };
        struct resampler_output_check_sink : sel::eng6::Processor1A0<ut_traits::output_frame_size>
        {
            size_t c = 0;

            void process() final
            {
                std::cout << "\n";
                for (size_t i = 0; i < 10; ++i)
                    std::cout << this->in_as_array(0)[i] << ' ';
                std::cout << "\n-----------\n";

                c += ut_traits::output_frame_size;
            }
        };

        void run() {

            sel::eng6::scheduler& s = sel::eng6::scheduler::get();
            s.clear();

            sel::eng6::proc::processor_graph graph(s);
//            sig_gen_ramp sig_gen;
            sig_gen_sine sig_gen;
            resampler_output_check_sink logger;
            resampler resampler;
            graph.connect(sig_gen, resampler);
            graph.connect(resampler, logger);

            s.init();
            sig_gen.raise(ut_traits::iters_to_run);
            for (size_t iter = 0; iter < ut_traits::iters_to_run; ++iter)
                s.step();

            const samp_t *results = logger.in_as_array(0);
        }

        void run_old() {


            sel::eng6::scheduler s = {};

            resampler resampler;
            sig_gen_ramp sig_gen_ramp;

            resampler_output_check_sink logger;


            sel::eng6::proc::compound_processor input_proc;
            sel::eng6::proc::compound_processor output_proc;


            input_proc.connect_procs(sig_gen_ramp, resampler.input_proc());
            output_proc.connect_procs(resampler.output_proc(), logger);

//	sel::eng6::semaphore sem(ut_traits::iters_to_run, rate_t(48000, 1));

//	sel::eng6::schedule input_schedule(&sem, input_proc);
            sel::eng6::periodic_event p1(rate_t(ut_traits::input_fs, ut_traits::input_frame_size));
            sel::eng6::schedule input_schedule(&p1, input_proc);
            sel::eng6::schedule output_schedule(&resampler, output_proc);

//	sem.raise(ut_traits::iters);
            s.add(input_schedule);
            s.add(output_schedule);
//	s.init();
            printf("\nInput schedule will run  %4.4f times\n", static_cast<double>(ut_traits::iters_to_run));

            s.do_measure_performance_at_start = true;
            s.run();
            auto input_rate_actual = static_cast<double>(input_schedule.actual_rate());
            auto fs_ratio = static_cast<double>(ut_traits::input_fs) / ut_traits::output_fs;
            auto correction = 1 / ( fs_ratio * fs_ratio); // ut_traits::input_frame_size / ( fs_ratio * fs_ratio);
            auto output_rate_actual = static_cast<double>(output_schedule.actual_rate());
            printf("\nInput rate\tClaimed: %4.4f\tActual: %4.4f\n", static_cast<double>(input_schedule.expected_rate()), input_rate_actual);
            printf("Output rate\tClaimed: %4.4f\tActual: %4.4f\n", static_cast<double>(output_schedule.expected_rate()), output_rate_actual * correction);

            // Resampler output after
            const samp_t *results = logger.in_as_array(0);
        }
SEL_UNIT_TEST_END

#endif

