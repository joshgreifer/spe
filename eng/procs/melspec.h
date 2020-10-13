#pragma once
// melspec class
#include "../melspec_impl.h"
#include "../processor.h"
#include "../factory.h"


namespace sel {
	namespace eng {
		namespace proc {

			template<typename traits, size_t NUM_FILTERS, size_t NUM_COEFFS = NUM_FILTERS>struct melspec_traits : traits
			{
				static constexpr size_t n_filters = NUM_FILTERS;
				static constexpr size_t n_coeffs = NUM_COEFFS;

			};

			template<class traits> class melspec :
				public  Processor1A1B<traits::input_frame_size / 2 + 1, traits::n_coeffs>, virtual public creatable<melspec<traits>>
			{
				
			private:
				melspec_impl<traits::input_fs, traits::n_filters, traits::input_frame_size> impl_;
			public:

				const std::string type() const final {
					char buf[100];
					snprintf(buf, 100, "melspec[%zd]", traits::n_coeffs);
					return buf;
				}

				void process() final {
					auto spectrum = this->in;
					auto coeffs = this->out;
					impl_.calculateMelFrequencySpectrum(spectrum, coeffs);

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