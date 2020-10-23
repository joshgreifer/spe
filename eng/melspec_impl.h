#pragma once

#include <cmath>
/**
	MEL spectrum implementation, derived from librosa's implementation.
	Given an fft magnitude spectrum, it produces a mel-scaled spectrum.
	
	Author: Josh Greifer
	Date:	20 Oct 2020

	melspec_impl<float, 16000, 80, 512>m;

	
	float fft_mag[257];  // fft magnitude
	float mel[80];	// output mel

	m.calculateMelFrequencySpectrum(fft_mag, mel);

	This is gives the same results as
	mel_filterbank = librosa.filters.mel(16000, 512, n_mels=80, fmin=0, fmax=8000, htk=True)
	mel = mel_filterbank.dot(fft_mag)
*/

template<class T, unsigned int sr, unsigned int nMels, unsigned int nFft, bool Htk=true, class internal_T=double>class melspec_impl
{
	static constexpr size_t real_spectrum_length = nFft / 2 + 1;
	static constexpr internal_T nyquist_frequency = sr / 2.0;
	static constexpr bool htk = Htk;

	std::vector<internal_T> calc_mel_frequencies(size_t n_mels = nMels + 2, internal_T fmin = 0, internal_T fmax = nyquist_frequency)
	{
		auto min_mel = frequency_to_mel_(fmin);
		auto max_mel = frequency_to_mel_(fmax);
		auto mels = sel::numpy::linspace<internal_T, internal_T>(min_mel, max_mel, n_mels);
		std::transform(mels.begin(), mels.end(), mels.begin(), [](auto v) { return mel_to_frequency(v);  });
		return mels;
	}

	
public:
	melspec_impl() : weights_( new std::array<double, nMels * real_spectrum_length>())
	{
		
		auto mel_f = calc_mel_frequencies();
		auto fft_freqencies = sel::numpy::linspace<internal_T, internal_T>(0.0, sr / 2.0, real_spectrum_length);
		// auto fdiff = sel::numpy::diff(mel_f);
		for (size_t i = 0; i < nMels; ++i)
		{
			auto m0 = mel_f[i];
			auto m1 = mel_f[i + 1];
			auto m2 = mel_f[i + 2];
			auto fdiff = m1 - m0;
			auto fdiff_1 = m2 - m1;
			auto enorm = 2.0 / (m2 - m0);
			for (size_t j = 0; j < real_spectrum_length; ++j)
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
	void calculateMelFrequencySpectrum(const T* inputMagnitudeSpectrum, T *outputMelFrequencySpectrum)

	{
		for (int i = 0; i < nMels; ++i)
		{
			internal_T coeff = 0;

			for (size_t j = 0; j < real_spectrum_length; ++j)
				coeff += inputMagnitudeSpectrum[j] * weights(i,j);

			outputMelFrequencySpectrum[i] = static_cast<T>(coeff);
		}

	}

	const auto& filterBank() const {
		return *weights_;
	}
private:

	mutable std::array<internal_T, nMels * real_spectrum_length> *weights_;
	
	internal_T& weights(size_t coeff, size_t specbin) const
	{
		return (*weights_)[coeff * real_spectrum_length + specbin];
	}
    template<bool=htk>static internal_T frequency_to_mel_(internal_T frequency);

    template<bool=htk>static internal_T mel_to_frequency(internal_T mel);

    // Use HTK formula
	template<>static internal_T frequency_to_mel_<true>(internal_T frequency)
	{
		return 2595.0 * log10(1.0 + frequency / 700.0);
	}

	// HTK
    template<>static internal_T mel_to_frequency<true>(internal_T mel)
	{
		return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);

	}
	// Slaney
    template<>static internal_T frequency_to_mel_<false>(internal_T frequency)
    {
// Fill in the linear part
        auto f_min = 0.0;
        auto f_sp = 200.0 / 3;

        auto mel = (frequency - f_min) / f_sp;

// Fill in the log-scale part

        auto min_log_hz = 1000.0;  // beginning of log region (Hz)
        auto min_log_mel = (min_log_hz - f_min) / f_sp;  // same (Mels)
        auto log_step = std::log(6.4) / 27.0;  //step size for log region

        if (frequency >= min_log_hz)
            return  min_log_mel + std::log(frequency / min_log_hz) / log_step;
        return mel;
    }

    // Use Slaney formula
    template<>static internal_T mel_to_frequency<false>(internal_T mel)
    {
        // Fill in the linear scale
        auto f_min = 0.0;
        auto f_sp = 200.0 / 3;
        auto freq = f_min + f_sp * mel;

// And now the nonlinear scale
        auto min_log_hz = 1000.0; // beginning of log region (Hz)
        auto min_log_mel = (min_log_hz - f_min) / f_sp; //same (Mels)
        auto log_step = std::log(6.4) / 27.0;  // step size for log region
        if (mel >= min_log_mel)
            return min_log_hz * std::exp(log_step * (mel - min_log_mel));
        return freq;
    }

};