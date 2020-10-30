#pragma once
// melspec class
#include "../melspec_impl.h"
#include "../processor.h"
#include "../factory.h"


namespace sel {
	namespace eng {
		namespace proc {

			template<class traits> class melspec :
				public  Processor1A1B<traits::input_frame_size, traits::n_mels>, virtual public creatable<melspec<traits>>
			{
				
			private:
				melspec_impl<double, traits::input_fs, traits::n_mels, traits::input_frame_size, traits::htk> impl_;
			public:

				const std::string type() const final {
					char buf[100];
					snprintf(buf, 100, "melspec[%zd]", traits::n_mels);
					return buf;
				}

				void process() final {
					auto spectrum = this->in;
					auto coeffs = this->out;
                    impl_.fft_mag2mel(spectrum, coeffs);
					

				}
				auto& filterBank() const
				{
					return impl_.filterBank();
				}
				melspec() = default;
				melspec(params& params) {}
			};
		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "melspec_ut.h"
#endif