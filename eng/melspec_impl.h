#pragma once

#include <cmath>
/*
	MEL spectrum implementation, derived from librosa's implementation
*/

template<unsigned int Fs, unsigned int Nmels, unsigned int Nffts>class melspec_impl
{
	static constexpr size_t spectrum_length = Nffts / 2 + 1;
	static constexpr double NyquistFrequency = Fs / 2.0;

	std::vector<double> calc_mel_frequencies(size_t n_mels = Nmels+2, double fmin = 0, double fmax = NyquistFrequency, bool htk=true)
	{
		auto min_mel = frequency_to_mel_(fmin);
		auto max_mel = frequency_to_mel_(fmax);
		auto mels = sel::numpy::linspace(min_mel, max_mel, n_mels);
		std::transform(mels.begin(), mels.end(), mels.begin(), [](auto v) { return mel_to_frequency(v);  });
		return mels;
	}

	
public:
	melspec_impl() : weights_( new std::array<double, Nmels * spectrum_length>())
	{
		
		auto mel_f = calc_mel_frequencies();
		auto fft_freqencies = sel::numpy::linspace(0.0, Fs / 2.0, spectrum_length);
		// auto fdiff = sel::numpy::diff(mel_f);
		for (size_t i = 0; i < Nmels; ++i)
		{
			auto m0 = mel_f[i];
			auto m1 = mel_f[i + 1];
			auto m2 = mel_f[i + 2];
			auto fdiff = m1 - m0;
			auto fdiff_1 = m2 - m1;
			auto enorm = 2.0 / (m2 - m0);
			for (size_t j = 0; j < spectrum_length; ++j)
			{
				const auto f = fft_freqencies[j];
				const auto ramp_i = m0 - f;
				const auto lower = -ramp_i / fdiff;
				
				const auto ramp_i_plus_2 = m2 - f;

				const auto upper = ramp_i_plus_2 / fdiff_1;
				auto w = std::max(0.0, std::min(lower, upper));
				// Slaney normalization
				w *= enorm;
				weights(i, j) = w;
			}
		}
		//calculate_mel_filter_bank_();
	}

	~melspec_impl()
	{
		delete weights_;
	}
	void calculateMelFrequencySpectrum(const double* inputMagnitudeSpectrum, double *outputMelFrequencySpectrum)

	{
		for (int i = 0; i < Nmels; i++)
		{
			double coeff = 0;

			for (size_t j = 0; j < spectrum_length; j++)
				coeff += (inputMagnitudeSpectrum[j] * inputMagnitudeSpectrum[j]) * weights(i,j);

			outputMelFrequencySpectrum[i] = coeff;
		}

	}

	const auto& filterBank() const {
		return *weights_;
	}
private:

	mutable std::array<double, Nmels * spectrum_length> *weights_;
	
	double& weights(size_t coeff, size_t specbin) const
	{
		return (*weights_)[coeff * spectrum_length + specbin];
	}

	static double frequency_to_mel_(double frequency, bool htk = true)
	{
		return 2595.0 * log10(1.0 + frequency / 700.0);
	}
	
	static double mel_to_frequency(double mel, bool htk = true)
	{
		return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);
	}
	/*
	void calculate_mel_filter_bank_()
	{
		auto maxMel = static_cast<int>(frequency_to_mel_(NyquistFrequency));
		auto minMel = static_cast<int>(frequency_to_mel_(0.0));

		for (size_t i = 0; i < Nmels; i++)
		{
			for (size_t j = 0; j < Nffts; j++)
				weights(i,j) = 0.0;
		}

		std::vector<int> centreIndices;

		for (size_t i = 0; i < Nmels + 2; ++i)
		{
			double f = i * (maxMel - minMel) / (Nmels + 1.0) + minMel;

			double tmp = log(1 + 1000.0 / 700.0) / 1000.0;
			tmp = (exp(f * tmp) - 1) / NyquistFrequency;
			tmp = 0.5 + 700.0 * Nffts * tmp;
			tmp = floor(tmp);

			int centreIndex = static_cast<int>(tmp);
			centreIndices.push_back(centreIndex);
		}

		for (size_t i = 0; i < Nmels; ++i)
		{
			int filterBeginIndex = centreIndices[i];
			int filterCenterIndex = centreIndices[i + 1];
			int filterEndIndex = centreIndices[i + 2];

			double triangleRangeUp = filterCenterIndex - filterBeginIndex;
			double triangleRangeDown = filterEndIndex - filterCenterIndex;

			// upward slope
			for (size_t k = filterBeginIndex; k < filterCenterIndex; k++)
				weights(i,k) = (k - filterBeginIndex) / triangleRangeUp;

			// downwards slope
			for (size_t k = filterCenterIndex; k < filterEndIndex; k++)
				weights(i,k) = (filterEndIndex - k) / triangleRangeDown;
		}

	}
	*/
};