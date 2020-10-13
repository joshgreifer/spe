// demo.cpp :Eng demo
//
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#endif

#define COMPILE_UNIT_TESTS

#include  "../eng/eng6.h"

struct demo_traits
{
	static constexpr size_t input_fs = 16000;
	static constexpr size_t output_fs = 16000;
	static constexpr size_t input_frame_size = 1024;
	static constexpr size_t hop_size = 256;
	static constexpr size_t overlap = input_frame_size - hop_size;
};

using wav_reader = sel::eng::proc::wav_file_data_source<1024>;
using hannwindow = sel::eng::proc::window_t<demo_traits, sel::eng::proc::wintype::HANN<demo_traits>, demo_traits::input_frame_size>;
using fft = sel::eng::proc::fftr_t<demo_traits>;



int main(int argc, const char *argv[])
{
	
	sel::eng::scheduler s = {};

	sel::eng::proc::compound_processor input_proc;
	sel::eng::proc::compound_processor output_proc;

	wav_reader wav_reader(argv[1]);
	hannwindow hannwindow;
	fft fft;
	auto win_out = hannwindow.output();
	input_proc.connect_procs(wav_reader, hannwindow.input());
	output_proc.connect_procs(hannwindow.output(), fft);

	sel::eng::schedule input_schedule(&wav_reader, input_proc);
	sel::eng::schedule output_schedule(&hannwindow, output_proc);
	

	s.add(input_schedule);
	s.add(output_schedule);

	return 0;
}

