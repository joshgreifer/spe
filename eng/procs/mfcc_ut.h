#pragma once

#include <iostream>
#include <array>

// lattice filter unit test

#include "rand.h"
#include "mfcc.h"
#include "fft.h"
#include "psd.h"
#include "compound_processor.h"

#include "window.h"
#include "matlab_file_output.h"

#include "../unit_test.h"

SEL_UNIT_TEST(mfcc)
	
struct ut_traits
{
	static constexpr size_t n_filters = 48;
	static constexpr size_t n_coeffs = 13;
	static constexpr size_t input_frame_size = 64;
	static constexpr size_t input_fs = 16000;
	static constexpr size_t overlap = 32;
	static constexpr size_t signal_length = 1000;
};

using mfcc = sel::eng::proc::mfcc<ut_traits>;
using hammingwindow = sel::eng::proc::window_t<ut_traits, sel::eng::proc::wintype::HAMMING<ut_traits>, 1>;
using fft = sel::eng::proc::fft_t<ut_traits>;
using psd = sel::eng::proc::psd<ut_traits>;

class preemphasis_filter : public sel::eng::Processor1A1B<1,1>
{
	samp_t prev_value = 0.0;
	const samp_t alpha_ = 0.97;
public:
	void process() final
	{
		this->out[0] = this->in[0] - prev_value * alpha_;
		prev_value = this->in[0];
	}	
};


void run() {


	hammingwindow window1;
	fft fft1;
	psd psd1;
	mfcc mfcc1;
	sel::eng::proc::rand<1> sig_gen_ramp;
	sel::uri mat_file_name = "file://" __FILE__ ".mat";
	cout << std::endl << mat_file_name.path() << std::endl;
	sel::eng::proc::matlab_file_output mat(mat_file_name);
//	sel::eng::proc::compound_processor in_chain({ &sig_gen, &window1.input() });
	sel::eng::proc::compound_processor input_chain({ &sig_gen_ramp, &window1.input() });
	sel::eng::proc::compound_processor output_chain({ &window1.output(), &fft1, &psd1, &mfcc1, &mat });
	sel::eng::schedule input_schedule(&sig_gen_ramp, input_chain);
	sel::eng::schedule frame_schedule(&window1, output_chain);

	input_schedule.init();
	frame_schedule.init();

	input_schedule.invoke(ut_traits::signal_length);

	input_schedule.term();
	frame_schedule.term();

//	std::cout << input_schedule;
//	std::cout << frame_schedule;
//	sel::eng::schedule(sig_gen, in_chain).invoke(ut_traits::signal_length);

}

SEL_UNIT_TEST_END




