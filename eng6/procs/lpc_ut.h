#pragma once

// lpc unit test
#include "lpc.h"

#include "../unit_test.h"

SEL_UNIT_TEST(lpc)

struct ut_traits
{
	static constexpr size_t sz = 3;
	static constexpr size_t num_coefficients = sz-1;

};
using lpc = sel::eng6::proc::lpc <ut_traits>;

std::array<double, ut_traits::sz> input_a = {{ 0.5, -0.7, 0.2 }};

sel::eng6::Const input = input_a;

std::array<samp_t, ut_traits::sz> matlab_levison_a_result = { 
	{ 1,	-0.875000000000000,	-1.62500000000000}
};
std::array<samp_t, ut_traits::sz-1> matlab_levison_k_result = { 
	{ 1.4,	-1.625 }
};

const samp_t matlab_levison_e_result = 0.7875;

void run() {

	lpc lpc;

	lpc.ConnectFrom(input);
	lpc.freeze();
	lpc.process();
	/*
	 * Matlab code: [a, e, k] = levinson([ 0.5, -.7, .2 ])
	 */
	// compare to Matlab
	for (size_t i = 0; i < ut_traits::sz; ++i) 
		SEL_UNIT_TEST_ASSERT(abs(lpc.a_out[i] - matlab_levison_a_result[i]) < 1e-10);
	for (size_t i = 0; i < ut_traits::sz-1; ++i) 
		SEL_UNIT_TEST_ASSERT(abs(lpc.k_out[i] - matlab_levison_k_result[i]) < 1e-10);

	SEL_UNIT_TEST_ASSERT(abs(*lpc.e_out - matlab_levison_e_result) < 1e-10);
	

	const auto my_lpc_ka_result = lpc.k_out;

}



SEL_UNIT_TEST_END
