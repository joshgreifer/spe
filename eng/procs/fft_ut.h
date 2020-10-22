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
using fft = sel::eng::proc::fft_t<ut_traits>;
using ifft = sel::eng::proc::ifft<ut_traits>;


static constexpr size_t  SZ = ut_traits::input_frame_size;

void run() {
	const auto seed = 5489U;
	sel::eng::proc::rand<SZ> rng(seed);

	fft fft1;
	rng.ConnectTo(fft1);

	rng.freeze();
	fft1.freeze();

	rng.process();
	fft1.process();

	csamp_t* my_fft_result = (csamp_t*)fft1.out;

	// TODO:: compare python fft



	ifft ifft1;
	fft1.ConnectTo(ifft1);
	ifft1.freeze();
	ifft1.process();

	csamp_t* my_ifft_result = (csamp_t*)ifft1.out;

	// compare to input
	for (size_t i = 0; i < SZ; ++i) {
		samp_t e = abs(my_ifft_result[i] - rng.out[i]);
		//if (e >= 1e-10)
		SEL_UNIT_TEST_ASSERT(e < 1e-10);

	}


}

SEL_UNIT_TEST_END

