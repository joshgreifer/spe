#pragma once

#include <iostream>
#include <array>

// lattice filter unit test

#include "rand.h"
#include "melspec.h"
#include "../numpy.h"
#include "fft.h"
#include "mag.h"
#include "compound_processor.h"

#include "window.h"
#include "matlab_file_output.h"

#include "../unit_test.h"

SEL_UNIT_TEST(melspec)
	
struct ut_traits
{
	static constexpr size_t n_filters = 80;
	static constexpr size_t n_mels = 80;
	static constexpr size_t input_frame_size = 1024;
	static constexpr size_t input_fs = 16000;
	static constexpr size_t overlap = 768;
	static constexpr size_t signal_length = 2048;
};

using melspec = sel::eng::proc::melspec<ut_traits>;
using hammingwindow = sel::eng::proc::window_t<ut_traits, sel::eng::proc::wintype::HAMMING<ut_traits>, 1>;
using fft = sel::eng::proc::fft_t<ut_traits>;
using mag = sel::eng::proc::mag<ut_traits>;

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
	auto& librosa = python::get().librosa;

	melspec melspec1;
	auto& filters = melspec1.filterBank();


	py::array_t<float> filters_py = librosa.attr("filters").attr("mel")(16000, 1024, "n_mels"_a = 80, "fmin"_a = 0, "fmax"_a = 8000, "htk"_a = true);
	auto filters_data = filters_py.data();
	auto filters_size = filters_py.size();
	SEL_UNIT_TEST_ASSERT(filters_py.size() == filters.size());
	for (size_t i = 0; i < filters.size(); ++i)
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filters[i], filters_data[i]);
}
#if 0
void run_old() {


	hamming_window window1;
	fft fft1;
	mag mag1;
	melspec melspec1;
	auto fb = melspec1.filterBank();

	sel::numpy::save(fb.data(), "mel_filterbank.npy", { ut_traits::n_filters, ut_traits::input_frame_size/2 + 1});
	sel::eng::proc::rand<1> sig_gen1;
	sel::uri mat_file_name = "file://" __FILE__ ".mat";
	cout << std::endl << mat_file_name.path() << std::endl;
	sel::eng::proc::matlab_file_output mat(mat_file_name);
	sel::eng::proc::compound_processor input_chain({ &sig_gen1, &window1.input() });
	sel::eng::proc::compound_processor output_chain({ &window1.output(), &fft1, &mag1, &melspec1, &mat });
	sel::eng::schedule input_schedule(&sig_gen1, input_chain);
	sel::eng::schedule frame_schedule(&window1, output_chain);
	auto filter_bank = melspec1.filterBank();
	
	input_schedule.init();
	frame_schedule.init();

	input_schedule.invoke(ut_traits::signal_length);

	input_schedule.term();
	frame_schedule.term();

	py::scoped_interpreter guard {};
	py::module np = py::module::import("numpy");
	py::module librosa = py::module::import("librosa");

	py::array_t<float> filters_py  = librosa.attr("filters").attr("mel")(16000, 1024, "n_mels"_a = 80, "fmin"_a = 0, "fmax"_a = 8000, "htk"_a = true);
	auto filters_data = filters_py.data();
	auto filters_size = filters_py.size();
	std::vector<float> vec = filters_py.cast<std::vector<float>>();

	vec[0];
	//py::array_t<size_t> shape_py = filters_py.attr("shape");
	//auto py_shape = shape_py.unchecked<1>();
	//auto vec = py_shape;
	//std::cout << shape_py[0];
	//py::exec(R"(
//print('Hello')
//)");
//	filters = librosa.filters.mel(16000, 1024, n_mels = 80, fmin = 0, fmax = 8000, htk = True)
//		print(filters.shape)
//	std::cout << input_schedule;
//	std::cout << frame_schedule;
//	sel::eng::schedule(sig_gen, in_chain).invoke(ut_traits::signal_length);

}
#endif
SEL_UNIT_TEST_END




