#pragma once
#include "../processor.h"
#include "../idx.h"
#include <vector>



namespace sel {
	namespace eng {
		namespace proc {


			template<size_t Sz=dynamic_size_v> 
			class fir_filt : public  Processor1A1B<1, 1>, virtual public creatable<fir_filt<Sz>>
			{
				const size_t sz;
				std::vector<samp_t> coeffs_;
				std::vector<samp_t> buf_;
				modulo_ptr<samp_t, Sz> buf_ptr_;

			public:
				explicit fir_filt() : sz(0) {}

				fir_filt(std::vector<samp_t>& coeffs) : 
				sz(coeffs.size()),  
				coeffs_(coeffs), 
				buf_(sz), buf_ptr_(sz)
				{
					buf_ptr_ = buf_.data();
				}

				fir_filt(std::initializer_list<samp_t> coeffs) :
				sz(coeffs.size()),  
				coeffs_(coeffs), 
				buf_(sz), buf_ptr_(sz)
				{
					buf_ptr_ = buf_.data();
				}
				fir_filt(params& args) : sz(0) {}

				virtual const std::string type() const final { return "fir filter"; }

			
				void process() final 
				{
					*buf_ptr_++  = this->in[0];
					double output = 0.0;
					for (auto coeff : coeffs_) {
						output += coeff * *buf_ptr_++;
					}

					this->out[0] = output;

				}	
			};

		} // proc
	} // eng
} // sel
#if (defined(COMPILE_UNIT_TESTS) || defined(UNIT_TEST_FIR_FILT))
#include "rand.h"
#include "../unit_test.h"

SEL_UNIT_TEST(fir_filt)
struct ut_traits
{
	static constexpr size_t signal_length = 10;
};
std::array<samp_t, ut_traits::signal_length> matlab_results = { {
-0.0567770431402061,
0.235800443449849,
0.622411425437107,
0.258499771265292,
0.274519584100127,
0.551486602116001,
0.184741622763833,
0.0557896312477389,
0.229308762658126,
0.465313382233125	
	} };

void run()
{
	sel::eng::proc::rand<1> rng;
	sel::eng::proc::fir_filt<4> filt({-0.0696887105265845,	0.366902203216131,	0.366902203216131,	-0.0696887105265845 });
	rng.ConnectTo(filt);
	rng.freeze();
	filt.freeze();

	for (size_t iter = 0; iter < ut_traits::signal_length; ++iter)
	{
		rng.process();
		filt.process();
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(filt.out[0], matlab_results[iter])

	}

	
}
SEL_UNIT_TEST_END
#endif


