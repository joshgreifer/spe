#pragma once
// lattice filter unit test
#include "lattice_filter.h"

#include "../unit_test.h"

SEL_UNIT_TEST(lattice_filter)

struct ut_traits
{
	static constexpr size_t num_coefficients = 3;
	static constexpr size_t iters = 5;

};

using latticefilter_ma = sel::eng6::proc::lattice_filter_t <ut_traits, sel::eng6::proc::MA_impl<ut_traits> > ;

static constexpr size_t NCOEFFS = ut_traits::num_coefficients;
static constexpr size_t ITERS = ut_traits::iters;

// output next value in vector until end, thereafter leave last value latched on output
struct sig_gen_ramp : sel::eng6::Processor01A<1>
{
	size_t i = 0;
	std::array<samp_t, ITERS> data_ = { { 1, 0, 0, 0, 0 } };
	void process() final 
	{

		out[0] = data_[i];
		if (i < ITERS -1)
			++i;
	}
} sig_gen_ramp;

sel::eng6::Const refection_coefficients = std::vector<double>(
	{
		1.0, 50.0, 0.1
	});


std::array<samp_t, ITERS> matlab_latticefilter_ma_result = { { 1.0, 56.0, 55.1, 0.1, 0.0 } };

void run() {


	latticefilter_ma latticefilter_ma;
	latticefilter_ma.ConnectFrom(sig_gen_ramp);
	latticefilter_ma.ConnectFrom(refection_coefficients, refection_coefficients.PORTID_DEFAULT, 1);
	sig_gen_ramp.freeze();
	latticefilter_ma.freeze();

	samp_t my_latticefilter_ma_result[ITERS];
	for (size_t step = 0; step < ITERS; ++step) {
		sig_gen_ramp.process();
		latticefilter_ma.process();
		my_latticefilter_ma_result[step] = *latticefilter_ma.out;
	}


	// compare matlab lattice filter
	/* (Matlab code:)
		function res = testit(k)

			analysisFilter = dsp.FIRFilter('Structure','Lattice MA', 'ReflectionCoefficients', k);


			res = [step(analysisFilter, 1) step(analysisFilter, 0) step(analysisFilter, 0) step(analysisFilter, 0) step(analysisFilter, 0) k];
		end
		>> testit([ 1 50 .1])

		ans = 
			1.0000   56.0000   55.1000    0.1000         0    1.0000   50.0000    0.1000

	   >>
		
	*/

	for (size_t i = 0; i < ITERS; ++i) {
		samp_t e = abs(my_latticefilter_ma_result[i] - matlab_latticefilter_ma_result[i]);
		//if (e >= 1e-10)
		SEL_UNIT_TEST_ASSERT(e < 1e-10);


	}
}

SEL_UNIT_TEST_END