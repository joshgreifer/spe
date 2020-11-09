#pragma once
#include "../processor.h"
#include "../idx.h"
#include <vector>



namespace sel {
	namespace eng {
		namespace proc {


			class iir_filt : public  Processor1A1B<1, 1>, virtual public creatable<iir_filt>
			{
				const size_t sz;
				std::vector<samp_t> b_;
				std::vector<samp_t> a_;
				std::vector<samp_t> w_;

				void check_coeffs() const
				{
				if (b_.size() != a_.size())
					throw eng_ex("IIR Filter: numerator (b) and denominator (a) coefficient vectors differ in size.");
				if (a_[0] == 0.0)
					throw eng_ex("IIR Filter: first denominator (a) coefficient cannot be zero.");
				}
					
				
			public:
				explicit iir_filt() : sz(0) {}

				iir_filt(std::vector<samp_t>& b, std::vector<samp_t>& a) : 
				sz(b.size()),  
				b_(b),
				a_(a), 
				w_(sz)
				{
					check_coeffs();
				}

				iir_filt(std::initializer_list<samp_t> b, std::initializer_list<samp_t> a) :
				sz(b.size()),  
				b_(b),
				a_(a), 
				w_(sz)
				{
					check_coeffs();
				}


				explicit iir_filt(params& args) : sz(0) {}

				virtual const std::string type() const final { return "iir filter"; }

			
				void process() final 
				{
					size_t i;
					samp_t y = 0;

					w_[0] = in[0];				// current input sample

					for ( i = 1; i < sz; ++i )	// input adder
						w_[0] -= a_[i] * w_[i];

					for ( i = 0; i < sz ; ++i )	// output adder
						y += b_[i] * w_[i];

					// now i == sz
					for (--i; i != 0; --i )		// shift buf backwards
						w_[i] = w_[i-1];
					
					out[0] =  y / a_[0];		// current output sample

				}	
			};

			struct preemphasis_filter : iir_filt
			{
				// pre-emphasis filter
				explicit preemphasis_filter(double alpha) :
				iir_filt({ 1, 0}, { 1.0, alpha }) {}
				
			};
			// See Hal Chamberlin's &quot;Musical Applications of Microprocessors, Page 585 ff &quot; for a discussion and explanation.
			// http://sites.music.columbia.edu/cmc/courses/g6610/fall2016/week8/Musical_Applications_of_Microprocessors-Charmberlin.pdf
			//6 dB per octave low pass (RC) filter.  This is implemented as an IIR filter, where the A and B coefficients are derived thus:

			//K = 1-exp(2PI Fc/Fs)
			//b = [ K/2 K/2 ]
			//a = [ 1 K-1 ]
			//

			template<size_t Fc, size_t Fs>struct lp_filter_6dB : iir_filt
			{
				static double k()
				{
					return 1.0 - exp(-2.0 * M_PI * Fc / Fs );
				}
				// lowpass  filter
				explicit lp_filter_6dB() :
				iir_filt(
					{ k() / 2.0, k() / 2.0 }, 
					{ 1.0, k() - 1.0 })  {}
				
			};

			template<size_t Fc, size_t Fs>struct hp_filter_6dB : iir_filt
			{
				static double k()
				{
					return 1.0 - exp(-2.0 * M_PI * Fc / Fs );
				}
				// highpass  filter
				explicit hp_filter_6dB() :
				iir_filt(
					{ 1.0 - k() / 2.0, k() / 2.0 - 1.0 }, 
					{ 1.0, k() - 1.0 })  {}
				
			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "rand.h"
#include "../unit_test.h"

SEL_UNIT_TEST(iir_filt)
struct ut_traits
{
	static constexpr size_t signal_length = 10;
};
std::array<samp_t, ut_traits::signal_length> matlab_results = { {
0.814723686393179,
0.115509961274236,
0.0149421538574975,
0.898881966897247,
-0.239556261664920,
0.329909978814382,
-0.0415144605829020,
0.587150545970399,
0.387970805843011,
0.588556853531556,
	} };

void run()
{
	sel::eng::proc::rand<1> rng;
	sel::eng::proc::iir_filt filt1({1, 0}, { 1, 0.97});
	sel::eng::proc::preemphasis_filter filt2(0.97);
	rng.ConnectTo(filt1);
	rng.ConnectTo(filt2);
	rng.freeze();
	filt1.freeze();
	filt2.freeze();

	for (size_t iter = 0; iter < ut_traits::signal_length; ++iter)
	{
		rng.process();
		filt1.process();
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filt1.out[0], matlab_results[iter])
		filt2.process();
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filt2.out[0], matlab_results[iter])

	}

	
}
SEL_UNIT_TEST_END
#endif



