#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

// window
template<size_t SZ>class sp_window
{
	// buffer for unmodifed input
	SIGNAL_T buf_[SZ];

	// index into input buffer
	size_t idx_ = 0;

	double hamming_factor(size_t i) { return 0.54f - 0.46f * cos((2.0 * M_PI * i) / (SZ - 1)); }

protected:
	virtual double window_func(size_t i) = 0;



	void process_(double newdata, double *outbuf)
	{
		buf_[idx_++] = newdata;  // idx_ now points to oldest data

		if (idx_ >= SZ) idx_ = 0;

		for (size_t i = 0, bi = idx_; i < SZ; ++i) {
			outbuf[i] = buf_[bi] * window_func(i);
			if (++bi == SZ) bi = 0;
		}
	}

	explicit sp_window() : idx_(0)
	{
		::memset(buf_, 0, sizeof(buf_));
	}

};

template<size_t SZ>class sp_hamming_window : public sp_window<SZ>, public  RegisterableSigProc<sp_hamming_window<SZ>, 1, SZ>
{
	double coeffs_[SZ];
	double window_func(size_t i) { return coeffs_[i]; }

public:
	PIN default_outpin() const { return ALL; }
	PIN default_inpin() const { return PIN1; }

	void process(void) { process_(**in, out); }
	// default constuctor needed for factory creation
	explicit sp_hamming_window() {}

	sp_hamming_window(params& args)
	{
		for (size_t i = 0; i < SZ; ++i)
			coeffs_[i] = 0.54f - 0.46f * cos((2.0 * M_PI * i) / (SZ - 1));
	}
};

template<size_t SZ>class sp_rectangular_window : public sp_window<SZ>, public  RegisterableSigProc<sp_rectangular_window<SZ>, 1, SZ>
{
	double window_func(size_t i) { return 1.0; }


public:
	PIN default_outpin() const { return ALL; }
	PIN default_inpin() const { return PIN1; }

	void process(void) { process_(**in, out); }
	// default constuctor needed for factory creation
	explicit sp_rectangular_window() {}

	sp_rectangular_window(params& args)
	{
	}


};
