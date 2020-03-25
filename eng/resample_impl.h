//RESAMPLE  Change the sampling rate of a signal.
//   Y = RESAMPLE(UpFactor, DownFactor, InputSignal, OutputSignal) resamples the sequence in 
//   vector InputSignal at UpFactor/DownFactor times and stores the resampled data to OutputSignal.
//   OutputSignal is UpFactor/DownFactor times the length of InputSignal. UpFactor and DownFactor must be 
//   positive integers.

//This function is translated from Matlab's Resample funtion. 

//Author: Haoqi Bai

// Usage: resampler_impl::resample(2048, 100, input, output);

#pragma once

//void resample ( int upFactor, int downFactor, 
//  vector<double>& inputSignal, vector<double>& outputSignal );


#include <boost/math/special_functions/bessel.hpp>

#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

/*
Copyright (c) 2009, Motorola, Inc

All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither the name of Motorola nor the names of its contributors may be
used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

using namespace std;

#include <stdexcept>
#include <complex>
#include <vector>
#include "ratio.h"

template<class S1, class S2, class C>
class Resampler {
public:
	typedef    S1 inputType;
	typedef    S2 outputType;
	typedef    double coefType;

	Resampler(int upRate, int downRate, C *coefs, int coefCount);
	virtual ~Resampler();

	int        apply(S1* in, int inCount, S2* out, int outCount);
	int        neededOutCount(int inCount);
	int        coefsPerPhase() { return _coefsPerPhase; }

private:
	int        _upRate;
	int        _downRate;

	coefType   *_transposedCoefs;
	inputType  *_state;
	inputType  *_stateEnd;

	int        _paddedCoefCount;  // ceil(len(coefs)/upRate)*upRate
	int        _coefsPerPhase;    // _paddedCoefCount / upRate

	int        _t;                // "time" (modulo upRate)
	int        _xOffset;

};


using std::invalid_argument;

template<class S1, class S2, class C>
Resampler<S1, S2, C>::Resampler(int upRate, int downRate, C *coefs,
	int coefCount) :
	_upRate(upRate), _downRate(downRate), _t(0), _xOffset(0)
	/*
	The coefficients are copied into local storage in a transposed, flipped
	arrangement.  For example, suppose upRate is 3, and the input number
	of coefficients coefCount = 10, represented as h[0], ..., h[9].
	Then the internal buffer will look like this:
	h[9], h[6], h[3], h[0],   // flipped phase 0 coefs
	0, h[7], h[4], h[1],   // flipped phase 1 coefs (zero-padded)
	0, h[8], h[5], h[2],   // flipped phase 2 coefs (zero-padded)
	*/
{
	_paddedCoefCount = coefCount;
	while (_paddedCoefCount % _upRate) {
		_paddedCoefCount++;
	}
	_coefsPerPhase = _paddedCoefCount / _upRate;

	_transposedCoefs = new coefType[_paddedCoefCount];
	fill(_transposedCoefs, _transposedCoefs + _paddedCoefCount, 0.);

	_state = new inputType[_coefsPerPhase - 1];
	_stateEnd = _state + _coefsPerPhase - 1;
	fill(_state, _stateEnd, 0.);


	/* This both transposes, and "flips" each phase, while
	* copying the defined coefficients into local storage.
	* There is probably a faster way to do this
	*/
	for (int i = 0; i<_upRate; ++i) {
		for (int j = 0; j<_coefsPerPhase; ++j) {
			if (j*_upRate + i  < coefCount)
				_transposedCoefs[(_coefsPerPhase - 1 - j) + i * _coefsPerPhase] =
				coefs[j*_upRate + i];
		}
	}
}

template<class S1, class S2, class C>
Resampler<S1, S2, C>::~Resampler() {
	delete[] _transposedCoefs;
	delete[] _state;
}

template<class S1, class S2, class C>
int Resampler<S1, S2, C>::neededOutCount(int inCount)
/* compute how many outputs will be generated for inCount inputs  */
{
	int np = inCount * _upRate;
	int need = np / _downRate;
	if ((_t + _upRate * _xOffset) < (np % _downRate))
		need++;
	return need;
}

template<class S1, class S2, class C>
int Resampler<S1, S2, C>::apply(S1* in, int inCount,
	S2* out, int outCount) {
	if (outCount < neededOutCount(inCount))
		throw invalid_argument("Not enough output samples");

	// x points to the latest processed input sample
	inputType *x = in + _xOffset;
	outputType *y = out;
	inputType *end = in + inCount;
	while (x < end) {
		outputType acc = 0.;
		coefType *h = _transposedCoefs + _t * _coefsPerPhase;
		inputType *xPtr = x - _coefsPerPhase + 1;
		int offset = (int)(in - xPtr);
		if (offset > 0) {
			// need to draw from the _state buffer
			inputType *statePtr = _stateEnd - offset;
			while (statePtr < _stateEnd) {
				acc += *statePtr++ * *h++;
			}
			xPtr += offset;
		}
		while (xPtr <= x) {
			acc += *xPtr++ * *h++;
		}
		*y++ = acc;
		_t += _downRate;

		int advanceAmount = _t / _upRate;

		x += advanceAmount;
		// which phase of the filter to use
		_t %= _upRate;
	}
	_xOffset = (int)(x - end);

	// manage _state buffer
	// find number of samples retained in buffer:
	int retain = (_coefsPerPhase - 1) - inCount;
	if (retain > 0) {
		// for inCount smaller than state buffer, copy end of buffer
		// to beginning:
		copy(_stateEnd - retain, _stateEnd, _state);
		// Then, copy the entire (short) input to end of buffer
		copy(in, end, _stateEnd - inCount);
	}
	else {
		// just copy last input samples into state buffer
		copy(end - (_coefsPerPhase - 1), end, _state);
	}
	// number of samples computed
	return (int)(y - out);
}

template<class S1, class S2, class C>
void upfirdn(int upRate, int downRate,
	const S1 *input, const int inLength, C *filter, const int filterLength,
	vector<S2> &results)
	/*
	This template function provides a one-shot resampling.  Extra samples
	are padded to the end of the input in order to capture all of the non-zero
	output samples.
	The output is in the "results" vector which is modified by the function.

	Note, I considered returning a vector instead of taking one on input, but
	then the C++ compiler has trouble with implicit template instantiation
	(e.g. have to say upfirdn<float, float, float> every time - this
	way we can let the compiler infer the template types).

	Thanks to Lewis Anderson (lkanders@ucsd.edu) at UCSD for
	the original version of this function.
	*/
{
	// Create the Resampler
	Resampler<S1, S2, C> theResampler(upRate, downRate, filter, filterLength);

	// pad input by length of one polyphase of filter to flush all values out
	int padding = theResampler.coefsPerPhase() - 1;
	S1 *inputPadded = new S1[inLength + padding];
	for (int i = 0; i < inLength + padding; i++) {
		if (i < inLength)
			inputPadded[i] = input[i];
		else
			inputPadded[i] = 0;
	}

	// calc size of output
	int resultsCount = theResampler.neededOutCount(inLength + padding);

	results.resize(resultsCount);

	// run filtering
	int numSamplesComputed = theResampler.apply(inputPadded,
		inLength + padding, &results[0], resultsCount);
	delete[] inputPadded;
}

template<class S1, class S2, class C>
void upfirdn(int upRate, int downRate,
	vector<S1> &input, vector<C> &filter, vector<S2> &results)
	/*
	This template function provides a one-shot resampling.
	The output is in the "results" vector which is modified by the function.
	In this version, the input and filter are vectors as opposed to
	pointer/count pairs.
	*/
{
	upfirdn<S1, S2, C>(upRate, downRate, &input[0], (int)input.size(), &filter[0],
		(int)filter.size(), results);
}

class resampler_impl
{
	const size_t input_size_;
	const sel::Ratio updown_ratio_;
	const int up_factor_;
	const int dn_factor_;
	vector<double> coeffs_;
	int delay_;
	const size_t ovec_size_;
public:
	resampler_impl(const size_t inputSize = 0, const size_t input_fs = 1, const size_t output_fs = 1)
		: 
		input_size_(inputSize),
		updown_ratio_(sel::Ratio(input_fs, output_fs).reduced()),
		up_factor_(static_cast<const int>(updown_ratio_.n())),
		dn_factor_(static_cast<const int>(updown_ratio_.d())),
		ovec_size_(sel::quotient_ceil(input_size_ * up_factor_, dn_factor_))
		
	{
		if (inputSize != 0)
		// Create filter - coeffs_ and delay_ are output args
			create_filter(static_cast<const int>(input_size_), up_factor_, dn_factor_, coeffs_, delay_);
	}
private:
	static int getGCD(int num1, int num2)
	{
		int tmp = 0;
		while (num1 > 0) {
			tmp = num1;
			num1 = num2 % num1;
			num2 = tmp;
		}
		return num2;
	}


	static double sinc(double x)
	{
		if (fabs(x - 0.0) < 0.000001)
			return 1;
		return sin(M_PI * x) / (M_PI * x);
	}

	static void firls(int length, vector<double> freq,
		const vector<double>& amplitude, vector<double>& result)
	{
		vector<double> weight;
		int freqSize = (int)freq.size();
		int weightSize = freqSize / 2;

		weight.reserve(weightSize);
		for (int i = 0; i < weightSize; i++)
			weight.push_back(1.0);

		int filterLength = length + 1;

		for (int i = 0; i < freqSize; i++)
			freq[i] /= 2.0;

		vector<double> dFreq;
		for (int i = 1; i < freqSize; i++)
			dFreq.push_back(freq[i] - freq[i - 1]);

		length = (filterLength - 1) / 2;
		int Nodd = filterLength % 2;
		double b0 = 0.0;
		vector<double> k;
		if (Nodd == 0) {
			for (int i = 0; i <= length; i++)
				k.push_back(i + 0.5);
		}
		else {
			for (int i = 0; i <= length; i++)
				k.push_back(i);
		}

		vector<double> b;
		int kSize = (int)k.size();
		for (int i = 0; i < kSize; i++)
			b.push_back(0.0);
		for (int i = 0; i < freqSize; i += 2) {
			double slope = (amplitude[i + 1] - amplitude[i]) / (freq[i + 1] - freq[i]);
			double b1 = amplitude[i] - slope * freq[i];
			if (Nodd == 1) {
				b0 += (b1 * (freq[i + 1] - freq[i])) +
					slope / 2.0 * (freq[i + 1] * freq[i + 1] - freq[i] * freq[i]) *
					fabs(weight[(i + 1) / 2] * weight[(i + 1) / 2]);
			}
			for (int j = 0; j < kSize; j++) {
				b[j] += (slope / (4 * M_PI * M_PI) *
					(cos(2 * M_PI * k[j] * freq[i + 1]) - cos(2 * M_PI * k[j] * freq[i])) / (k[j] * k[j])) *
					fabs(weight[(i + 1) / 2] * weight[(i + 1) / 2]);
				b[j] += (freq[i + 1] * (slope * freq[i + 1] + b1) * sinc(2 * k[j] * freq[i + 1]) -
					freq[i] * (slope * freq[i] + b1) * sinc(2 * k[j] * freq[i])) *
					fabs(weight[(i + 1) / 2] * weight[(i + 1) / 2]);
			}
		}
		if (Nodd == 1)
			b[0] = b0;
		vector<double> a;
		double w0 = weight[0];
		for (int i = 0; i < kSize; i++)
			a.push_back((w0 * w0) * 4 * b[i]);
		if (Nodd == 1) {
			a[0] /= 2;
			for (int i = length; i >= 1; i--)
				result.push_back(a[i] / 2.0);
			result.push_back(a[0]);
			for (int i = 1; i <= length; i++)
				result.push_back(a[i] / 2.0);
		}
		else {
			for (int i = length; i >= 0; i--)
				result.push_back(a[i] / 2.0);
			for (int i = 0; i <= length; i++)
				result.push_back(a[i] / 2.0);
		}
	}

	static void kaiser(const int order, const double bta, vector<double>& window)
	{
		double bes = fabs(boost::math::cyl_bessel_i(0, bta));
		int odd = order % 2;
		double xind = (order - 1) * (order - 1);
		int n = (order + 1) / 2;
		vector<double> xi;
		xi.reserve(n);
		for (int i = 0; i < n; i++) {
			double val = static_cast<double>(i) + 0.5 * (1 - static_cast<double>(odd));
			xi.push_back(4 * val * val);
		}
		vector<double> w;
		w.reserve(n);
		for (int i = 0; i < n; i++)
			w.push_back(boost::math::cyl_bessel_i(0, bta * sqrt(1 - xi[i] / xind)) / bes);
		for (int i = n - 1; i >= odd; i--)
			window.push_back(fabs(w[i]));
		for (int i = 0; i < n; i++)
			window.push_back(fabs(w[i]));
	}

	static void hamming(const int order, vector<double>& window)
	{

		for (int i = 0; i < order; i++)
			window.push_back(0.54f - 0.46f * cos((2.0 * M_PI * i) / (order - 1)));
	}

	// Create filter.  upfactor and downfactor may get changed (divided by gcd)
	// the coefficents are returned in h.

	void create_filter(const int inputSize, const int upFactor, const int downFactor, vector<double>& h, int& delay)
	{
		const int n = 10;
		const double bta = 5.0;
		//if (upFactor <= 0 || downFactor <= 0)
		//	throw std::runtime_error("factors must be positive integer");
		//int gcd = getGCD(upFactor, downFactor);
		//upFactor /= gcd;
		//downFactor /= gcd;

		const auto outputSize = sel::quotient_ceil(inputSize * upFactor, downFactor);

		const int maxFactor = max(upFactor, downFactor);
		const double firlsFreq = 1.0 / 2.0 / static_cast<double> (maxFactor);
		const int length = 2 * n * maxFactor + 1;
		double firlsFreqs[] = { 0.0, 2.0 * firlsFreq, 2.0 * firlsFreq, 1.0 };
		vector<double> firlsFreqsV;
		firlsFreqsV.assign(firlsFreqs, firlsFreqs + 4);
		double firlsAmplitude[] = { 1.0, 1.0, 0.0, 0.0 };
		vector<double> firlsAmplitudeV;
		firlsAmplitudeV.assign(firlsAmplitude, firlsAmplitude + 4);
		vector<double> coefficients;
		firls(length - 1, firlsFreqsV, firlsAmplitudeV, coefficients);
		vector<double> window;
		kaiser(length, bta, window);
		// hamming(length, window);
		const int coefficientsSize = (int)coefficients.size();
		for (int i = 0; i < coefficientsSize; i++)
			coefficients[i] *= upFactor * window[i];

		int lengthHalf = (length - 1) / 2;
		int nz = downFactor - lengthHalf % downFactor;
		h.reserve(coefficientsSize + nz);
		for (int i = 0; i < nz; i++)
			h.push_back(0.0);
		for (int i = 0; i < coefficientsSize; i++)
			h.push_back(coefficients[i]);
		const int hSize = (int)h.size();
		lengthHalf += nz;
		delay = lengthHalf / downFactor;
		nz = 0;
		while (sel::quotient_ceil((inputSize - 1) * upFactor + hSize + nz, downFactor) - delay < static_cast<int>(outputSize))
			nz++;
		for (int i = 0; i < nz; i++)
			h.push_back(0.0);
	}

public:
	size_t output_frame_size()  const { return ovec_size_; }

	void resample(const double *inputSignal, double * outputSignal)
	{
		// int outputSize = quotientCeil(inputSize * upFactor, downFactor);

		vector<double> y;
		upfirdn(up_factor_, dn_factor_, inputSignal, static_cast<int>(input_size_), coeffs_.data(), static_cast<const int>(coeffs_.size()), y);
		for (size_t i = 0; i < ovec_size_; i++) {
			outputSignal[i] = y[i + delay_];
		}
	}

};
