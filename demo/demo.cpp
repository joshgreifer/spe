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
	static constexpr bool htk = true;

};

using wav_reader = sel::eng::proc::wav_file_data_source<demo_traits::input_frame_size>;
using hann_window = sel::eng::proc::window_t<demo_traits, sel::eng::proc::wintype::HANN<demo_traits>, demo_traits::input_frame_size>;
using no_window = sel::eng::proc::window_t<demo_traits, sel::eng::proc::wintype::RECTANGULAR<demo_traits>, demo_traits::input_frame_size>;
using fft = sel::eng::proc::fft_t<demo_traits>;
using mag = sel::eng::proc::mag<demo_traits>;
using mel = sel::eng::proc::melspec<demo_traits>;


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

	sel::eng::proc::numpy_file_writer<samp_t, demo_traits::input_frame_size> numpy_writer_audio("test_audio_16k_i16.wav.npy");
	sel::eng::proc::numpy_file_writer<samp_t, demo_traits::n_mels> numpy_writer_mel("test_audio_16k_i16.mels.npy");
	sel::eng::proc::numpy_file_writer<samp_t, demo_traits::input_frame_size> numpy_writer_frames("test_audio_16k_i16.frames.npy");
	sel::eng::proc::numpy_file_writer<samp_t, demo_traits::input_frame_size> numpy_writer_random("test_audio_16k_i16.rng.npy");

	sel::eng::proc::rand<demo_traits::input_frame_size> rng;
	rng.raise(1);  // Run once only

	sel::eng::proc::processor_graph graph(s);

	graph.connect(wav_reader, window);

	graph.connect(window, fft1);
	graph.connect(fft1, mag);
	graph.connect(mag, mel);

	// Write the mels to numpy file
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

