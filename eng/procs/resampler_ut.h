#pragma once


// resampler unit tests
#include "resampler.h"
#include "samples.h"
#include "../unit_test.h"

SEL_UNIT_TEST(resampler)

sel::eng::scheduler scheduler = {};

struct ut_traits
{

	static constexpr size_t input_fs = 48000;
	static constexpr size_t output_fs = 16000;
	static constexpr size_t input_frame_size = 256;
	static constexpr size_t output_frame_size = 256;
	static constexpr size_t seconds_to_run = 15;
	static constexpr size_t iters_to_run = static_cast<size_t>(seconds_to_run * input_fs / static_cast<double>(input_frame_size));

};
using resampler = sel::eng::proc::resampler <ut_traits, 
	ut_traits::output_fs, 
	ut_traits::input_frame_size,
	ut_traits::output_frame_size>;

struct sig_gen_ramp : sel::eng::Processor01A<ut_traits::input_frame_size>
{

	size_t c = 0;
	size_t iters_remaining = ut_traits::iters_to_run;

	sel::eng::scheduler& scheduler;

	sig_gen_ramp(sel::eng::scheduler& scheduler) : scheduler(scheduler) {}
	
	void process() final
	{
		if (iters_remaining-- == 0)
			scheduler.stop();
		for (size_t i = 0; i < ut_traits::input_frame_size; ++i)
			out[i] = static_cast<samp_t>(c++);
	}
};

struct resampler_output_check_sink : sel::eng::Processor1A0<ut_traits::output_frame_size>
{
	size_t c = 0;
	
	void process() final
	{
		c += ut_traits::output_frame_size;
	}
};


void run() {


	sel::eng::scheduler s = {};

	resampler resampler;
	sig_gen_ramp sig_gen_ramp(s);

	resampler_output_check_sink logger;


	sel::eng::proc::compound_processor input_proc;
	sel::eng::proc::compound_processor output_proc;


	input_proc.connect_procs(sig_gen_ramp, resampler.input_proc());
	output_proc.connect_procs(resampler.output_proc(), logger);

//	sel::eng::semaphore sem(ut_traits::iters_to_run, rate_t(48000, 1));

//	sel::eng::schedule input_schedule(&sem, input_proc);
	sel::eng::periodic_event p1(rate_t(ut_traits::input_fs, ut_traits::input_frame_size));
	sel::eng::schedule input_schedule(&p1, input_proc);
	sel::eng::schedule output_schedule(&resampler, output_proc);

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
