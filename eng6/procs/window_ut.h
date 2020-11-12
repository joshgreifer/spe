#pragma once
#include <iostream>

// window unit test
#include "window.h"
#include "wav_file_data_source.h"
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

struct ut_traits
{
	static constexpr size_t input_fs = 16000;
	static constexpr size_t output_fs = 16000;
	static constexpr size_t input_frame_size = 1024;
	static constexpr size_t hop_size = 256;
	static constexpr size_t overlap = input_frame_size - hop_size;

};
struct sig_gen_ramp : sel::eng6::Processor01A<ut_traits_overlap::input_frame_size>
{
	size_t c = 0;
	void process() final
	{
		for (size_t i = 0; i < ut_traits_overlap::input_frame_size; ++i)
			out[i] = static_cast<samp_t>(c++);
	}
} sig_gen_ramp;

using kaiser_window = sel::eng6::proc::window_t<ut_traits_no_overlap, sel::eng6::proc::wintype::KAISER<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using hamming_window = sel::eng6::proc::window_t<ut_traits_no_overlap, sel::eng6::proc::wintype::HAMMING<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using rectangular_window = sel::eng6::proc::window_t<ut_traits_overlap, sel::eng6::proc::wintype::RECTANGULAR<ut_traits_overlap>, ut_traits_overlap::input_frame_size>;
using hann_window = sel::eng6::proc::window_t<ut_traits, sel::eng6::proc::wintype::HANN<ut_traits>, ut_traits::input_frame_size>;

using wav_reader = sel::eng6::proc::wav_file_data_source<ut_traits::input_frame_size>;

sel::eng6::Const input = std::vector<double>(ut_traits_no_overlap::input_frame_size, 1.0);
static constexpr size_t N = ut_traits_no_overlap::input_frame_size;



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


	kaiser_window kaiser_window;
	kaiser_window.ConnectFrom(input);
	kaiser_window.freeze();
	kaiser_window.process();

	const auto my_kaiser_result = kaiser_window.out;
	SEL_UNIT_TEST_ITEM("kaiser window");
	// compare matlab fft
	for (size_t i = 0; i < N; ++i) 
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(my_kaiser_result[i], matlab_kaiser_result[i]);

	


	//sel::eng6::proc::sample::Logger logger;
	struct null_sink : sel::eng6::Processor1A0<ut_traits_overlap::input_frame_size>
	{
		void process() final {}
	} logger;

	rectangular_window rectangular_window;
	const auto input_rate = rate_t(ut_traits_overlap::input_fs, ut_traits_overlap::input_frame_size);
	sel::eng6::semaphore sem(0, input_rate);
	
	sel::eng6::proc::compound_processor input_proc;
	sel::eng6::proc::compound_processor output_proc;

	input_proc.connect_procs(sig_gen_ramp, rectangular_window.input_proc());
	output_proc.connect_procs(rectangular_window.output_proc(), logger);


	sel::eng6::scheduler s = {};
	sel::eng6::schedule input_schedule(&sem, input_proc);
	sel::eng6::schedule output_schedule(&rectangular_window, output_proc);

	sem.raise(ut_traits_overlap::iters);
	s.add(input_schedule);
	s.add(output_schedule);
	s.init();

	static constexpr auto hop_length = ut_traits_overlap::input_frame_size - ut_traits_overlap::overlap;

	const rate_t overlap_factor(ut_traits_overlap::input_frame_size, hop_length);
	SEL_UNIT_TEST_ITEM("overlap window");
	SEL_UNIT_TEST_ITEM("rate");
	SEL_UNIT_TEST_ASSERT(output_schedule.expected_rate() == input_rate * overlap_factor);

	SEL_UNIT_TEST_ITEM("hop_length");
	while (s.step()) {
		
		//for (auto v:  rectangular_window.oport)
		//	std::cerr << v << ' ';
		//std::cerr << std::endl;
		
		SEL_UNIT_TEST_ASSERT(static_cast<size_t>(rectangular_window.out[0] + 0.5) % hop_length == 0);
	}


}

SEL_UNIT_TEST_END

