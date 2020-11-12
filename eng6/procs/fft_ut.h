#pragma once
// fft unit test
#include "fft.h"
#include "rand.h"
#include <iostream>
#include "../unit_test.h"

SEL_UNIT_TEST(fft)

struct ut_traits
{
	static constexpr size_t input_frame_size = 1024;
};
using fft = sel::eng6::proc::fft_t<ut_traits>;
using ifft = sel::eng6::proc::ifft<ut_traits>;


static constexpr size_t  SZ = ut_traits::input_frame_size;

void run() {

	sel::eng6::proc::rand<SZ> rng;

	fft fft1;
	rng.ConnectTo(fft1);

	rng.freeze();
	fft1.freeze();

	rng.process();
	fft1.process();

	csamp_t* my_fft_result = (csamp_t*)fft1.out;

	SEL_UNIT_TEST_ITEM("np.fft.fft");
	auto py_np_fft_fft = python::get().np.attr("fft").attr("fft");
	py::array_t<double> rand_py(SZ, rng.out);
	py::array_t<csamp_t> fft_py = py_np_fft_fft(rand_py);
	auto fft_py_vec = python::make_vector_from_1d_numpy_array(fft_py);
	for (size_t i = 0; i < SZ; ++i) {
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(fft_py_vec[i].real(), my_fft_result[i].real());
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(fft_py_vec[i].imag(), my_fft_result[i].imag());
	}
	ifft ifft1;
	fft1.ConnectTo(ifft1);
	ifft1.freeze();
	ifft1.process();

	csamp_t* my_ifft_result = (csamp_t*)ifft1.out;

	// compare to input
	SEL_UNIT_TEST_ITEM("invert (ifft)");
	for (size_t i = 0; i < SZ; ++i)
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(my_ifft_result[i].real(), rng.out[i]);



}

SEL_UNIT_TEST_END

