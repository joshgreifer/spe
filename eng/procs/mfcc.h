#pragma once
// mfcc class
#include "../mfcc_impl.h"
#include "../processor.h"
#include "../factory.h"


namespace sel {
	namespace eng {
		namespace proc {

			template<typename traits, size_t NUM_FILTERS, size_t NUM_COEFFS = NUM_FILTERS>struct mfcc_traits : traits
			{
				static constexpr size_t n_filters = NUM_FILTERS;
				static constexpr size_t n_coeffs = NUM_COEFFS;

			};

			template<class traits> class mfcc :
				public  Processor1A1B<traits::input_frame_size / 2 + 1, traits::n_coeffs>, virtual public creatable<mfcc<traits>>
			{
				static constexpr size_t spectrum_length = traits::input_frame_size / 2;
			private:
				mfcc_impl<traits::input_fs, traits::n_filters, spectrum_length, traits::n_coeffs> impl_;
			public:

				const std::string type() const final {
					char buf[100];
					snprintf(buf, 100, "mfcc[%zd]", traits::n_coeffs);
					return buf;
				}

				void process() final {
					auto spectrum = this->in;
					auto coeffs = this->out;
					impl_.CalcCoefficients(spectrum, coeffs);

				}

				mfcc() {}
				mfcc(params& params) {}
			};
		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "mfcc_ut.h"
#endif