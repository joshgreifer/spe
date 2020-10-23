#pragma once
#include "../processor.h"

namespace sel {
	namespace eng {
		namespace proc {

			template<class traits, size_t SZ=traits::input_frame_size, size_t FS=traits::input_fs>struct psd : public Processor1A1B<2 * SZ, SZ/2+1>, virtual public creatable<psd<traits, SZ, FS> >
			{
				// input is an fft
				static constexpr size_t OUTW = SZ / 2 + 1;
			public:
				virtual const std::string type() const override { return "power spectral density"; }

				void process() final
				{
					csamp_t *in_as_complex_array = (csamp_t *)this->in;
					for (size_t i = 0; i < OUTW; ++i) {
						samp_t v = abs(in_as_complex_array[i]);
						v = 1.0 / (FS * SZ) * v * v;
						if (i > 0 && i < SZ / 2)
							v *= 2;
						this->out[i] = v;

					}
				}
				// default constuctor needed for factory creation
				explicit psd() {}

				psd(params& args)
				{
				}


			};
		} // proc
	} // eng
} //sel


#if defined(COMPILE_UNIT_TESTS)
#include "psd_ut.h"
#endif