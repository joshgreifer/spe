#pragma once
#include "../processor.h"
/*
 * Input: complex FFT, output: magnitude
 */
namespace sel {
	namespace eng {
		namespace proc {

			template<class traits, size_t SZ = traits::input_frame_size>
			class mag :
			public Processor1A1B<2 * SZ, SZ / 2 + 1>,
			virtual public creatable<mag<traits, SZ> >
			{
				// input is an fft
				static constexpr size_t OUTW = SZ / 2 + 1;
			public:
				const std::string type() const final { return "magnitude"; }

				void process() final
				{
					const auto in_as_complex_array = reinterpret_cast<const csamp_t*>(this->in);
					for (size_t i = 0; i < OUTW; ++i) {
						samp_t v = abs(in_as_complex_array[i]);
						this->out[i] = v;

					}
				}
				// default constructor needed for factory creation
				explicit mag() {}

				mag(params& args)
				{
				}


			};
		} // proc
	} // eng
} //sel


#if defined(COMPILE_UNIT_TESTS)
// #include "mag_ut.h"
#endif