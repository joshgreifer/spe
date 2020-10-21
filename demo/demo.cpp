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


#include  "../eng/eng6.h"

struct demo_traits
{
	static constexpr size_t input_fs = 16000;
	static constexpr size_t output_fs = 16000;
	static constexpr size_t input_frame_size = 1024;
	static constexpr size_t hop_size = 256;
	static constexpr size_t overlap = input_frame_size - hop_size;
	static constexpr size_t n_mels = 80;

};

using wav_reader = sel::eng::proc::wav_file_data_source<demo_traits::input_frame_size>;
using hann_window = sel::eng::proc::window_t<demo_traits, sel::eng::proc::wintype::HANN<demo_traits>, demo_traits::input_frame_size>;
using no_window = sel::eng::proc::window_t<demo_traits, sel::eng::proc::wintype::RECTANGULAR<demo_traits>, demo_traits::input_frame_size>;
using fft = sel::eng::proc::fft_t<demo_traits>;
using mag = sel::eng::proc::mag<demo_traits>;
using mel = sel::eng::proc::melspec<demo_traits>;



int main_old(int argc, const char *argv[])
{
	const char* file_name = argc > 1 ? argv[1] : "test_audio_16k_i16.wav";
	sel::eng::scheduler s = {};

	sel::eng::proc::compound_processor input_proc;
	sel::eng::proc::compound_processor output_proc;

	wav_reader wav_reader(file_name);
	no_window window;
	fft fft;
	mag mag;
	mel mel;
	sel::eng::proc::matlab_file_output mat("mels.mat");
	
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size> numpy_writer_audio("test_audio_16k_i16.wav.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::n_mels> numpy_writer_mel("test_audio_16k_i16.mels.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size / 2 + 1> numpy_writer_mag("test_audio_16k_i16.mag.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size> numpy_writer_frames("test_audio_16k_i16.frames.npy");

	auto& win_out = window.output_proc();
	input_proc.connect_procs(wav_reader, window.input_proc());
	output_proc.connect_procs(window.output_proc(), fft);
	output_proc.connect_procs(fft, mag);
	output_proc.connect_procs(mag, mel);
	
	// Write the mag and mels to numpy files
	output_proc.connect_procs(mag, numpy_writer_mag);
	output_proc.connect_procs(mel, numpy_writer_mel);
	// Write out the audio and windowed frames to numpy files
	input_proc.connect_procs(wav_reader, numpy_writer_audio);
	output_proc.connect_procs(win_out, numpy_writer_frames);
	
	sel::eng::schedule input_schedule(&wav_reader, input_proc);
	sel::eng::schedule output_schedule(&window, output_proc);
	

	s.add(input_schedule);
	output_schedule.init();
	//s.add(output_schedule);


	s.run();
	output_schedule.term();

	return 0;
}

int main(int argc, const char* argv[])
{
	const char* file_name = argc > 1 ? argv[1] : "test_audio_16k_i16.wav";
	sel::eng::scheduler s = {};



	wav_reader wav_reader(file_name);
	no_window window;
	fft fft1;
	fft fft_rand;
	mag mag;
	mel mel;
	sel::eng::proc::matlab_file_output mat("mels.mat");

	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size> numpy_writer_audio("test_audio_16k_i16.wav.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::n_mels> numpy_writer_mel("test_audio_16k_i16.mels.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size / 2 + 1> numpy_writer_mag("test_audio_16k_i16.mag.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size> numpy_writer_frames("test_audio_16k_i16.frames.npy");
	sel::eng::proc::numpy_file_writer<demo_traits::input_frame_size> numpy_writer_random("test_audio_16k_i16.rng.npy");

	sel::eng::proc::rand<demo_traits::input_frame_size> rng;
	rng.raise(1);  // Run once only

	sel::eng::proc::processor_graph graph(s);

	graph.connect(wav_reader, window);

	graph.connect(window, fft1);
	graph.connect(fft1, mag);
	graph.connect(mag, mel);

	// Write the mag and mels to numpy files
	graph.connect(mag, numpy_writer_mag);
	graph.connect(mel, numpy_writer_mel);
	// Write out the audio and windowed frames to numpy files
	graph.connect(wav_reader, numpy_writer_audio);
	graph.connect(window, numpy_writer_frames);

	// write the random number to numpy file
	graph.connect(rng, numpy_writer_random);
	graph.connect(rng, fft_rand);

	s.run();

	return 0;
}

