#pragma once
#include <iostream>

// window unit test
#include "window.h"
#include "compound_processor.h"
#include "../scheduler.h"
#include "../unit_test.h"

SEL_UNIT_TEST(window)

struct ut_traits_no_overlap
{
	static constexpr size_t input_frame_size = 8;
	static constexpr size_t overlap = 0;
	static constexpr double kaiser_beta = 5.0;
};

struct ut_traits_overlap
{
	static constexpr size_t input_frame_size = 10;
	static constexpr size_t overlap = 3;
	static constexpr size_t iters = 10;
	static constexpr size_t input_fs = 16000;
};

struct sig_gen_ramp : sel::eng::Processor01A<ut_traits_overlap::input_frame_size>
{
	size_t c = 0;
	void process() final
	{
		for (size_t i = 0; i < ut_traits_overlap::input_frame_size; ++i)
			out[i] = static_cast<samp_t>(c++);
	}
} sig_gen_ramp;

using kaiserwindow = sel::eng::proc::window_t<ut_traits_no_overlap, sel::eng::proc::wintype::KAISER<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using hammingwindow = sel::eng::proc::window_t<ut_traits_no_overlap, sel::eng::proc::wintype::HAMMING<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using rectangularwindow = sel::eng::proc::window_t<ut_traits_overlap, sel::eng::proc::wintype::RECTANGULAR<ut_traits_overlap>, ut_traits_overlap::input_frame_size>;


sel::eng::Const input = std::vector<double>(ut_traits_no_overlap::input_frame_size, 1.0);
static constexpr size_t N = ut_traits_no_overlap::input_frame_size;

std::array<samp_t, N> matlab_hamming_result = { {
	0.0800000000000000,
	0.253194691144983,
	0.642359629619905,
	0.954445679235113,
	0.954445679235113,
	0.642359629619905,
	0.253194691144983,
	0.0800000000000000			}
};

std::array<samp_t, N> matlab_kaiser_result = { {
	0.0367108922712867,
	0.270694417889416,
	0.651738235245363,
	0.955247316456436,
	0.955247316456436,
	0.651738235245363,
	0.270694417889416,
	0.0367108922712867
}
};


void run() {

	hammingwindow hamming_window;
	hamming_window.ConnectFrom(input);
	hamming_window.freeze();
	hamming_window.process();

	const auto my_hamming_result = hamming_window.out;

	// compare matlab fft
	for (size_t i = 0; i < N; ++i) {
		auto e = abs(my_hamming_result[i] - matlab_hamming_result[i]);
		//if (e >= 1e-10)
		SEL_UNIT_TEST_ASSERT(e < 1e-10);

	}

	kaiserwindow kaiser_window;
	kaiser_window.ConnectFrom(input);
	kaiser_window.freeze();
	kaiser_window.process();

	const auto my_kaiser_result = kaiser_window.out;

	// compare matlab fft
	for (size_t i = 0; i < N; ++i) {
		auto e = abs(my_kaiser_result[i] - matlab_kaiser_result[i]);
		//if (e >= 1e-10)
		SEL_UNIT_TEST_ASSERT(e < 1e-10);

	}


	//sel::eng::proc::sample::Logger logger;
	struct null_sink : sel::eng::Processor1A0<ut_traits_overlap::input_frame_size>
	{
		void process() final {}
	} logger;

	rectangularwindow rectangular_window;
	const auto input_rate = rate_t(ut_traits_overlap::input_fs, ut_traits_overlap::input_frame_size);
	sel::eng::semaphore sem(0, input_rate);
	
	sel::eng::proc::compound_processor input_proc;
	sel::eng::proc::compound_processor output_proc;

	input_proc.connect_procs(sig_gen_ramp, rectangular_window.input());
	output_proc.connect_procs(rectangular_window.output(), logger);


	sel::eng::scheduler s = {};
	sel::eng::schedule input_schedule(&sem, input_proc);
	sel::eng::schedule output_schedule(&rectangular_window, output_proc);

	sem.raise(ut_traits_overlap::iters);
	s.add(input_schedule);
	s.add(output_schedule);
	s.init();

	static constexpr auto stride = ut_traits_overlap::input_frame_size - ut_traits_overlap::overlap;

	const rate_t overlap_factor(ut_traits_overlap::input_frame_size, stride);

	SEL_UNIT_TEST_ASSERT(output_schedule.expected_rate() == input_rate * overlap_factor);

	while (s.step()) {
		
		for (auto v:  rectangular_window.output().oport)
			std::cerr << v << ' ';
		std::cerr << std::endl;
		
		SEL_UNIT_TEST_ASSERT(static_cast<size_t>(rectangular_window.output().out[0] + 0.5) % stride == 0);
	}


}

SEL_UNIT_TEST_END

