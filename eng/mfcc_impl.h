#pragma once
/*
* libmfcc.c - Code implementation for libMFCC
* Copyright (c) 2010 Jeremy Sawruk
*
* This code is released under the MIT License.
* For conditions of distribution and use, see the license in LICENSE
*/

#include <math.h>
//#include "libmfcc.h"
/*
* libmfcc.h - Header for libMFCC
* Copyright (c) 2010 Jeremy Sawruk
*
* This code is released under the MIT License.
* For conditions of distribution and use, see the license in LICENSE
*/


// Returns the specified (mth) MFCC
//double GetCoefficient(double* spectralData, unsigned int samplingRate, unsigned int NumFilters, unsigned int binSize, unsigned int m);

// Compute the normalization factor (For internal computation only - not to be called directly)
//double NormalizationFactor(int NumFilters, int m);

// Compute the filter parameter for the specified frequency and filter bands (For internal computation only - not the be called directly)
//double GetFilterParameter(unsigned int samplingRate, unsigned int binSize, unsigned int frequencyBand, unsigned int filterBand);

// Compute the band-dependent magnitude factor for the given filter band (For internal computation only - not the be called directly)
//double GetMagnitudeFactor(unsigned int filterBand);

// Compute the center frequency (fc) of the specified filter band (l) (For internal computation only - not the be called directly)
//double GetCenterFrequency(unsigned int filterBand);

template<unsigned int samplingRate, unsigned int NumFilters, unsigned int binSize, unsigned int NumCoeffs>class mfcc_impl
{
	
	static constexpr double PI = 3.14159265358979323846264338327;

public:
	mfcc_impl()
	{
		for (unsigned int filtband = 1; filtband <= NumFilters; ++filtband)
			for (unsigned int bin = 0; bin < binSize; ++bin)
				FilterParameter[bin][filtband] = GetFilterParameter(bin, filtband);
	}
	/*
	* Computes the specified (mth) MFCC
	*
	* spectralData - array of doubles containing the results of FFT computation. This data is already assumed to be purely real
	* samplingRate - the rate that the original time-series data was sampled at (i.e 44100)
	* NumFilters - the number of filters to use in the computation. Recommended value = 48
	* binSize - the size of the spectralData array, usually a power of 2
	* m - The mth MFCC coefficient to compute
	*
	*/
	void CalcCoefficients(const double* spectralData, double *coeffs)

	{

//		double innerSum = 0.0;
//		unsigned int k, l;

		// precalc innersum partial calc, which is the same for all m


		double innerSums[NumFilters + 1];
		for (unsigned int l = 1; l <= NumFilters; l++) {
			double innerSum = 0.0;
			for (unsigned int k = 0; k < binSize - 1; k++) {
				innerSum += fabs(spectralData[k] * FilterParameter[k][l]);

			}
			if (innerSum > 0.0)
				innerSum = log(innerSum); // The log of 0 is undefined, so don't use it

			innerSums[l] = innerSum;
		}

		for (unsigned int m = 0; m < NumCoeffs; ++m) {

			double outerSum = 0.0;

			for (unsigned int l = 1; l <= NumFilters; l++)
				outerSum += innerSums[l] * cos(((m * PI) / NumFilters) * (l - 0.5));
			

			coeffs[m] = outerSum * NormalizationFactor(m);
		}

	}
private:
	/*
	* Computes the Normalization Factor (Equation 6)
	* Used for internal computation only - not to be called directly
	*/
	const double NormalizationFactor_IF_EQ_0 = sqrt(1.0 / NumFilters);
	const double NormalizationFactor_IF_NE_0 = sqrt(2.0 / NumFilters);

	inline constexpr double NormalizationFactor(unsigned int m)
	{
		return m ? NormalizationFactor_IF_NE_0 : NormalizationFactor_IF_EQ_0;

	}

	double FilterParameter[binSize][NumFilters+1];

	/*
	* Compute the filter parameter for the specified frequency and filter bands (Eq. 2)
	* Used for internal computation only - not the be called directly
	*/
	double GetFilterParameter(unsigned int frequencyBand, unsigned int filterBand)
	{
		double filterParameter = 0.0;

		double boundary = (frequencyBand * samplingRate) / binSize;		// k * Fs / N
		double prevCenterFrequency = GetCenterFrequency(filterBand - 1);		// fc(l - 1) etc.
		double thisCenterFrequency = GetCenterFrequency(filterBand);
		double nextCenterFrequency = GetCenterFrequency(filterBand + 1);

		if (boundary >= 0 && boundary < prevCenterFrequency)
		{
			filterParameter = 0.0;
		}
		else if (boundary >= prevCenterFrequency && boundary < thisCenterFrequency)
		{
			filterParameter = (boundary - prevCenterFrequency) / (thisCenterFrequency - prevCenterFrequency);
			filterParameter *= GetMagnitudeFactor(filterBand);
		}
		else if (boundary >= thisCenterFrequency && boundary < nextCenterFrequency)
		{
			filterParameter = (boundary - nextCenterFrequency) / (thisCenterFrequency - nextCenterFrequency);
			filterParameter *= GetMagnitudeFactor(filterBand);
		}
		else if (boundary >= nextCenterFrequency && boundary < samplingRate)
		{
			filterParameter = 0.0;
		}

		return filterParameter;
	}

	/*
	* Compute the band-dependent magnitude factor for the given filter band (Eq. 3)
	* Used for internal computation only - not the be called directly
	*/
	double GetMagnitudeFactor(unsigned int filterBand)
	{
		double magnitudeFactor = 0.0;

		if (filterBand >= 1 && filterBand <= 14)
		{
			magnitudeFactor = 0.015;
		}
		else if (filterBand >= 15 && filterBand <= 48)
		{
			magnitudeFactor = 2.0 / (GetCenterFrequency(filterBand + 1) - GetCenterFrequency(filterBand - 1));
		}

		return magnitudeFactor;
	}

	/*
	* Compute the center frequency (fc) of the specified filter band (l) (Eq. 4)
	* This where the mel-frequency scaling occurs. Filters are specified so that their
	* center frequencies are equally spaced on the mel scale
	* Used for internal computation only - not the be called directly
	*/
	double GetCenterFrequency(unsigned int filterBand)
	{

		if (filterBand == 0)
			return 0.0;
		
		else if (filterBand <= 14)
		
			return 200.0 * filterBand / 3.0;
		
		// else 
		const double exponent = filterBand - 14.0;
		const double logstep = exp(log(6.4)/27);
		return pow(logstep, exponent) * 1073.4;

		

	}
};