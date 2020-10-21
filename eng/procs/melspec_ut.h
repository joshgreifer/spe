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
	static constexpr size_t overlap = 0;
	//static constexpr size_t signal_length = 2048;
};

using melspec = sel::eng::proc::melspec<ut_traits>;
using hann_window = sel::eng::proc::window_t<ut_traits, sel::eng::proc::wintype::HANN<ut_traits>, ut_traits::input_frame_size>;
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

template<class T> vector<T>make_vector_from_1d_numpy_array(py::array_t<T>py_array)
{
	return std::vector<T>(py_array.data(), py_array.data() + py_array.size());
}

void run() {
	auto librosa = python::get().librosa;
	auto np = python::get().np;
	auto py_np_random = np.attr("random");
	
	melspec melspec1;
	auto& filters = melspec1.filterBank();


	py::array_t<float> filters_py = librosa.attr("filters").attr("mel")(16000, 1024, "n_mels"_a = 80, "fmin"_a = 0, "fmax"_a = 8000, "htk"_a = true);
	auto filters_data = filters_py.data();
	auto filters_size = filters_py.size();
	SEL_UNIT_TEST_ITEM("melspec.filterBank() filters size");
	SEL_UNIT_TEST_ASSERT(filters_py.size() == filters.size())
	SEL_UNIT_TEST_ITEM("melspec.filterBank() filters data");
	for (size_t i = 0; i < filters.size(); ++i)
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filters[i], filters_data[i])
	
	const auto seed = 5489U;
	sel::eng::proc::rand<ut_traits::input_frame_size> rng1(seed);
	hann_window window1;
	fft fft1;
	mag mag1;

	sel::eng::scheduler& s = sel::eng::scheduler::get();
	
	sel::eng::proc::processor_graph graph(s);
	graph.connect(rng1, window1);
	graph.connect(window1, fft1);
	graph.connect(fft1, mag1);
	graph.connect(mag1, melspec1);

	rng1.raise();  // run one schedule
	s.init();
	s.step();

	// compare with librosa
	SEL_UNIT_TEST_ITEM("melspec process()");

	py_np_random.attr("seed")(seed);
	// py::array_t<double> rand_py = py_np_random.attr("random")(ut_traits::input_frame_size);
	py::array_t<double> rand_py(ut_traits::input_frame_size, rng1.out);
// mel2 = librosa.feature.melspectrogram(y=y, sr=16000,  n_mels=80, fmin=0, fmax=8000,
// center=False, n_fft=1024, htk=True, window=np.ones(1024), hop_length=1024, power=1)

	auto librosa_melspectrogram = librosa.attr("feature").attr("melspectrogram");
	py::array_t<double>melspec_py = librosa_melspectrogram("y"_a = rand_py,
		"sr"_a = ut_traits::input_fs,
		"n_mels"_a = ut_traits::n_mels,
		"fmin"_a = 0,
		"fmax"_a = ut_traits::input_fs / 2,
		"center"_a = false,
		"n_fft"_a = ut_traits::input_frame_size,
		"htk"_a = true,  // This implementation only supports htk style mel banks
		"hop_length"_a = ut_traits::input_frame_size,
		"power"_a = 1,
		"window"_a = "hann"
	);
	auto melspec_py_data = make_vector_from_1d_numpy_array(melspec_py);
	for (size_t i = 0; i < ut_traits::n_mels; ++i)
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(melspec_py_data[i], melspec1.out[i]);
}

SEL_UNIT_TEST_END




