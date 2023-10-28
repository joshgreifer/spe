#pragma once
#include "../processor.h"
#include "../idx.h"
#include <vector>



namespace sel {
	namespace eng6 {
		namespace proc {


			template<size_t SZ>class iir_filt : public  Processor1A1B<SZ, SZ>
			{
				const size_t n_coeffs;
				const std::vector<samp_t> b_;
				const std::vector<samp_t> a_;
				std::vector<samp_t> w_;

				void check_coeffs() const
				{
				if (b_.size() != a_.size())
					throw eng_ex("IIR Filter: numerator (b) and denominator (a) coefficient vectors differ in size.");
				if (a_[0] == 0.0)
					throw eng_ex("IIR Filter: first denominator (a) coefficient cannot be zero.");
				}
					
				
			public:

				iir_filt(std::vector<samp_t> b, std::vector<samp_t> a) :
                        n_coeffs(b.size()),
                        b_(b),
                        a_(a),
                        w_(n_coeffs)
				{
					check_coeffs();
				}

				iir_filt(std::initializer_list<samp_t> b, std::initializer_list<samp_t> a) :
                        n_coeffs(b.size()),
                        b_(b),
                        a_(a),
                        w_(n_coeffs)
				{
					check_coeffs();
				}

			    iir_filt(const BiquadCoeffs& coeffs) :  iir_filt(
                        std::vector<double>(coeffs.b.begin(), coeffs.b.end()) ,
                        std::vector<double>(coeffs.a.begin(), coeffs.a.end()))
                        {

                }

                iir_filt operator+(const iir_filt& other) {
                    return iir_filt(poly_multiply(b_, other.b_), poly_multiply(a_, other.a_));
                }

				void process() final 
				{
					size_t j;

                    for (size_t i = 0; i < SZ; ++i) {

                        samp_t y = 0;

                        w_[0] = iir_filt<SZ>::in[i];				// current input sample

                        for (j = 1; j < n_coeffs; ++j )	// input adder
                            w_[0] -= a_[j] * w_[j];

                        for (j = 0; j < n_coeffs ; ++j )	// output adder
                            y += b_[j] * w_[j];

                        // now i == sz
                        for (--j; j != 0; --j )		// shift buf backwards
                            w_[j] = w_[j - 1];

                        iir_filt<SZ>::out[i] =  y / a_[0];		// current output sample

                    }

				}	
			};

			template<size_t SZ>struct preemphasis_filter : iir_filt<SZ>
			{
				// pre-emphasis filter
				explicit preemphasis_filter(double alpha) :
				iir_filt<SZ>({ 1, 0}, { 1.0, alpha }) {}
				
			};
			// See Hal Chamberlin's &quot;Musical Applications of Microprocessors, Page 585 ff &quot; for a discussion and explanation.
			// http://sites.music.columbia.edu/cmc/courses/g6610/fall2016/week8/Musical_Applications_of_Microprocessors-Charmberlin.pdf
			//6 dB per octave low pass (RC) filter.  This is implemented as an IIR filter, where the A and B coefficients are derived thus:

			//K = 1-exp(2PI Fc/Fs)
			//b = [ K/2 K/2 ]
			//a = [ 1 K-1 ]
			//

			template<size_t SZ, size_t Fc, size_t Fs>struct lp_filter_6dB : iir_filt<SZ>
			{
				static double k()
				{
					return 1.0 - exp(-2.0 * M_PI * Fc / Fs );
				}
				// lowpass  filter
				explicit lp_filter_6dB() :
				iir_filt<SZ>(
					{ k() / 2.0, k() / 2.0 }, 
					{ 1.0, k() - 1.0 })  {}
				
			};

			template<size_t SZ, size_t Fc, size_t Fs>struct hp_filter_6dB : iir_filt<SZ>
			{
				static double k()
				{
					return 1.0 - exp(-2.0 * M_PI * Fc / Fs );
				}
				// highpass  filter
				explicit hp_filter_6dB() :
				iir_filt<SZ>(
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
	sel::eng6::proc::rand<ut_traits::signal_length> rng;
	sel::eng6::proc::iir_filt<ut_traits::signal_length> filt1({1, 0}, { 1, 0.97});
	sel::eng6::proc::preemphasis_filter<ut_traits::signal_length> filt2(0.97);
	rng.ConnectTo(filt1);
	rng.ConnectTo(filt2);
	rng.freeze();
	filt1.freeze();
	filt2.freeze();


    rng.process();

    filt1.process();
    for (size_t i = 0; i < ut_traits::signal_length; ++i)
        SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filt1.out[i], matlab_results[i]);
    filt2.process();
    for (size_t i = 0; i < ut_traits::signal_length; ++i)
        SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filt2.out[i], matlab_results[i]);



	
}
SEL_UNIT_TEST_END
#endif



