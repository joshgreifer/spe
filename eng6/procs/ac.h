#pragma once

#include "../processor.h"
#include "fft.h"

namespace sel {
	namespace eng6 {
		namespace proc {
			template<typename traits=eng_traits<> >struct ac : public  Processor1A1B<2 * traits::input_frame_size, traits::input_frame_size>, virtual public creatable<ac<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				using baseProcessor = Processor1A1B<2 * SZ, SZ>;

				ifft<traits> ifft_;
				// input is an fft
			public:

				void process(void)
				{
					csamp_t *fft_as_complex_array = (csamp_t *)this->in;
					// get mag multiply each point by its complex conjugate (will reduce imag to 0.0 if input is real)
					for (size_t i = 0; i < SZ; ++i)
						fft_as_complex_array[i] *= conj(fft_as_complex_array[i]);

					ifft_.process();

					csamp_t *ifft_as_complex_array = (csamp_t *)ifft_.out;
					// output real values only, normalized
					for (size_t i = 0; i < SZ; ++i)
						this->out[i] = ifft_as_complex_array[i].real() / (samp_t)SZ;

				}
				// default constructor needed for factory creation
				explicit ac() {}

				void freeze(void) final 
				{
					ifft_.ConnectInputToInput(*this);
					ifft_.freeze();
					baseProcessor::freeze();
				}

				ac(params& args)
				{
				}


			};
		} // proc
	} // eng
} //sel
#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"

SEL_UNIT_TEST(ac)
SEL_UNIT_TEST_END
#endif
