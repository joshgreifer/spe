/*
Exponentially Weighted Moving Average

*/
#pragma once
#include "../processor.h"

namespace sel
{
	namespace eng6 {
		namespace proc 
		{
			template<size_t Fs>class ewma : public ScalarProc
			{
				
				const double alpha_ = 0.0;

				samp_t s_ = NO_SIGNAL; // current ema

			public:
				// https://books.google.co.uk/books?id=Zle0_-zk1nsC&pg=PA797&lpg=PA797
				// https://pandas.pydata.org/pandas-docs/version/0.17.0/generated/pandas.ewma.html
				static constexpr double half_life_to_alpha(double half_life)
				{
					return 1.0 - exp(log(0.5) / (half_life * Fs));
					//return 2 / (2.8854 * half_life * Fs + 1);
				}
				
				void process() final
				{
					if (isnan(s_))  // first time
						s_ = *this->in;
					else
						s_ = *this->in * alpha_ + s_ * (1.0 - alpha_);

					*this->out = s_;
				}
				// default constructor needed for factory creation
				ewma() = default;

				explicit ewma(double alpha) : ScalarProc(), alpha_(alpha)
				{
				}


				explicit ewma(params& args) : ewma( half_life_to_alpha(args.get<double>("half-life-seconds")))
				{
					
				}

			};
		}
	}
}

#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"

using namespace sel::eng6::proc;

SEL_UNIT_TEST(ewma)
	struct ut_traits
	{
		static constexpr size_t half_life_seconds = 10;
		static constexpr size_t input_fs = 10000;
	};

		sel::params create_params = {{ "half-life-seconds", "10.0" }};
void run()
{
	// Feed an impulse to the ewma -- should decay to 0.5 after 'half_life_seconds' seconds
	sel::eng6::Const input = 1.0;
	ewma<ut_traits::input_fs> ewma1(ewma<ut_traits::input_fs>::half_life_to_alpha(ut_traits::half_life_seconds));
	input.ConnectTo(ewma1);
	ewma1.freeze();
	ewma1.process();
	// set input to no signal now
	input = 0.0;
	for (size_t i = 1; i < ut_traits::input_fs * ut_traits::half_life_seconds; ++i)
		ewma1.process();
	
	SEL_UNIT_TEST_EQUAL_THRESH(*ewma1.out, 0.5, 1e-4)

}
SEL_UNIT_TEST_END
#endif

