#pragma once
#include "../processor.h"

namespace sel {
	namespace eng6 {
		namespace proc {

			// spectrogram
			//
			// https://uk.mathworks.com/help/signal/ug/power-spectral-density-estimates-using-fft.html

			// input is an fft 
			// do power spectral density, scale to decibels,  make range 0.0 .. 1.0

			template<size_t SZ, size_t FS>struct spectrogram : public Processor1A1B<2 * SZ, SZ / 2 + 1>, virtual public creatable<spectrogram<SZ, FS> >
			{
				virtual const std::string type() const override { return "spectrogram"; }

				static constexpr size_t OUTW = SZ / 2 + 1;

				samp_t min_dB;
				samp_t max_dB;

				samp_t max_freq; // freq with most power

			public:

				void process(void)
				{
					csamp_t *in_as_complex_array = (csamp_t *)this->in;
					samp_t *out = this->out;

					samp_t min = min_dB;
					samp_t max = max_dB;
					samp_t max_freq_bin = -1;

					for (int i = 0; i < OUTW; ++i) {
						samp_t v = abs(in_as_complex_array[i]);
						v = 1.0 / (FS * SZ) * v * v;
						if (i > 0 && i < SZ / 2)
							v *= 2;
						samp_t temp = 10.0 * log10(v);
						out[i] = temp;
						if (temp > max) { max = temp; max_freq_bin = i; }
						if (temp < min) min = temp;
					}
					// normalize to max
					for (int i = 0; i < OUTW; ++i) {
						out[i] -= min;
						out[i] /= (max - min);
					}
					if (min_dB > min) min_dB = min;
					if (max_dB < max) {
						max_dB = max;
						max_freq = max_freq_bin / SZ * FS;
					}
				}
				// default constuctor needed for factory creation
				explicit spectrogram() {}

				spectrogram(params& args)
				{
					max_dB = args.get<double>("max", -DBL_MAX);
					min_dB = args.get<double>("min", DBL_MAX);
					max_freq = 0.0;
				}


			};
		} // proc
	} // eng
} //sel
