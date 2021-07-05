#pragma once
#include "../processor.h"

namespace sel {
	namespace eng6 {
		namespace proc {
            // Discrete Cosine Transform, type II]

            // https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
            // This code follows SciPy's dct II method
            // https://docs.scipy.org/doc/scipy/reference/generated/scipy.fftpack.dct.html
			template<class traits, size_t SZ = traits::input_frame_size>struct dct : public Processor1A1B<SZ, SZ>, virtual public creatable<dct<traits, SZ> >
			{
				static constexpr size_t N = SZ;
				double dctSignal[SZ];
			public:
				virtual const std::string type() const override { return "discrete cosine transform"; }

				void process() final
				{

					const auto piOverN = M_PI / N;

					for (size_t k = 0; k < SZ; ++k) {

						double& sum = this->out[k];
                        sum = 0;
						for (size_t n = 0; n < SZ; n++)
							sum += this->in[n] * cos(piOverN * (n + 0.5) * k);

						sum *= 2.0;
					}

				}
				// default constructor needed for factory creation
				explicit dct() {}

				dct(params& args)
				{
				}

			};
		} // proc
	} // eng
} //sel


#if defined(COMPILE_UNIT_TESTS)
#pragma once
#include "../unit_test.h"

SEL_UNIT_TEST(dct)

class ut_traits : public eng_traits<5, 16000> {};



using dct = sel::eng6::proc::dct<ut_traits>;


sel::eng6::Const input = std::vector<double>(
	{1, 2, 3, 4, 6});

std::array<samp_t, dct::N> scipy_dct_result = { {
    32.        ,
    -11.86170617,
    1.61803399,
    -2.07362646,
    0.61803399

	} };


void run() {

	dct dct;

	dct.ConnectFrom(input);


	dct.freeze();


	dct.process();

	samp_t* my_dct_result = dct.out;

	// compare scipy dct

	for (size_t i = 0; i < dct::N; ++i)
	    SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(my_dct_result[i], scipy_dct_result[i]);


}

SEL_UNIT_TEST_END
#endif