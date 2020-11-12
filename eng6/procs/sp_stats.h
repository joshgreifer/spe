#pragma once
#include "sp.h"
#include <float.h>
#include <set>
/*
	Running stats
*/



#define CALC_MEDIAN 0
// every RECALC_SUM_FREQ calcuations, recalculate the buffer sum to avoid cumulative errors
#define RECALC_SUM_FREQ 100000


class sp_stats : public SigProc, public iRunTimeModifiable
{
	size_t _idx;
	SIGNAL_T *buf;
	size_t N;
	size_t new_size;
	SIGNAL_T sum;
	SIGNAL_T sum_sqrs; // Sum of squared values (for energy/power calc)
	double meanX, b2; // see linear regression.  These values are precalculated


	double sample_interval;
	bool initialized;

	bool calc_var;
	bool calc_gradient;

	int recalc_counter;

#if (CALC_MEDIAN)
	std::multiset<SIGNAL_T>::iterator median_ptr;
	std::multiset<SIGNAL_T> _median_buf;
#endif

public:
	const char* name() const { return "Stats Buffer"; }
	static const PIN MEAN = PIN1;
	static const PIN VAR = PIN2;
	static const PIN STDDEV = PIN3;
	static const PIN MAX = PIN4;
	static const PIN MIN = PIN5;
	static const PIN GRADIENT = PIN6;
	static const PIN ZERO_CROSSING = PIN7;
	static const PIN ENERGY = PIN8;
	static const PIN POWER = PIN9;

	PIN default_inpin() const { return PIN1; }
	PIN default_outpin() const { return MEAN; }


	sp_stats(size_t size)
		: SigProc(1,9),
		N(size),
		new_size(N),
		_idx(0),
		sum(0),
		sum_sqrs(0),
		sample_interval(0),
		initialized(false),
		calc_var(false),
		calc_gradient(false),
		recalc_counter(0)


	{
		if (size <= 1)
			throw exception("Stats buffer size must be greater than one");

		buf = new SIGNAL_T[size]; memset(buf, 0, sizeof(SIGNAL_T) * size);

#if (CALC_MEDIAN)
		_median_buf.clear();
		for (size_t i = 0; i < size; ++i)
			_median_buf.insert(0.0);

		median_ptr = _median_buf.begin();

		for (size_t c = 0; c < size/2; ++c) {
			++median_ptr;
		}
#endif		
		meanX = (size-1) / 2.0;
		b2 = 0.0;
		for (size_t i = 0; i < size; ++i) {
			b2 += (i - meanX) * (i - meanX);
		}

		// median_ptr now points to  middle element of median_buf
	}

	virtual ~sp_stats(void)
	{
		delete[] buf;
	}
	
	void init(Schedule *context)
	{
		checkinputs();

		// stats optimization flags
		calc_var = is_output_connected(VAR) || is_output_connected(STDDEV);
		calc_gradient = is_output_connected(GRADIENT);

		sample_interval = (double)context->trigger->get_duration();
	}

	void Modify(params& args) 
	{
		size_t tnew_size = N;

		if (is_arg(args, "interval")) {
			if (sample_interval == 0)
				throw exception("Can't set stats interval, because the sample rate is zero.");
			double new_interval =  mand_arg<double>(args, "interval");
			if (new_interval < sample_interval * 2)
				throw exception("Stats interval must be greater than twice the sampling interval.");
		
			new_size = (size_t)(new_interval / sample_interval);
		}

		if (is_arg(args, "size")) {
			tnew_size = mand_arg<int>(args, "size");
			if (tnew_size <= 1)
				throw exception("Stats buffer size must be greater than one");
			else
				new_size = tnew_size;
		}

	}

	void process()
	{
		// if size has been modified, reset the buffers.
		// This has to be done here, because Modify() is called asynchronously

		// 20080709 -- Above comment is wrong.  Modify is called *synchronously*.  But
		// The code below works, and is used to initialize the state of the processor,
		// so I won't move it into the Modify() method.
		if (new_size != N) {
			N = new_size;
			delete[] buf;
			buf = new SIGNAL_T[N]; memset(buf, 0, sizeof(SIGNAL_T) * N);
			_idx = 0;
			sum = sum_sqrs = 0;
#if (CALC_MEDIAN)
			_median_buf.clear();
			for (size_t i = 0; i < new_size; ++i)
				_median_buf.insert(0.0);

			median_ptr = _median_buf.begin();

			for (size_t c = 0; c < new_size/2; ++c) {
				++median_ptr;
			}
#endif			
			meanX = (N-1) / 2.0;
			b2 = 0.0;
			for (size_t i = 0; i < N; ++i) {
				b2 += (i - meanX) * (i - meanX);
			}

			initialized = false;
		}
		const SIGNAL_T v = *in[0];
		
		const SIGNAL_T oldest_v = buf[_idx];

		sum -= oldest_v;
		sum_sqrs -= oldest_v * oldest_v;
		buf[_idx] = v; 

		// update sum
		if (++recalc_counter >= RECALC_SUM_FREQ) {
			recalc_counter = 0;
			sum = sum_sqrs = 0.0;
			for (size_t i = 0; i < N; ++i) {
				const SIGNAL_T v = buf[i];
				sum += v;
				sum_sqrs += v * v;
			}
		} else {
			sum += v;
			sum_sqrs += v * v;
		}
		size_t n = initialized ? N : _idx+1;

		SIGNAL_T mean = sum / n;

		// TODO: Implement better algorithm See http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance

		double _max = -DBL_MAX, _min = DBL_MAX;
		double var = 0.0;
		
		double b1 = 0.0;

		size_t zc = 0;

		for (size_t i = 0; i < n; ++i) {

			SIGNAL_T vv = buf[i];
			// zero crossing
			if (i > 0)
				if ((vv > 0.0) != (buf[i - 1] > 0.0))
					++zc;

			// linear regression
			if (calc_gradient) {
				size_t idx = _idx+i; if (idx >= n) idx -= n;
				b1 += (i - meanX) * (buf[idx] - mean);
			}
			// var
			if (calc_var) {
				SIGNAL_T diffmean = vv - mean;
				SIGNAL_T diffmean_squared = diffmean * diffmean;
				var += diffmean_squared;
			}
			// min, max
			if (vv > _max)
				_max = vv;
			if (vv < _min)
				_min  =vv;
		}
		// normalize var
		var /= (n-1);

		// linear regression gradient coefficient
		double b = b1 / b2;
		
#if (CALC_MEDIAN)
		// median
		_median_buf.insert(v);
		if (*median_ptr > v) {
			_median_buf.erase(--_median_buf.end());
			--median_ptr;
		} else {
			_median_buf.erase(_median_buf.begin());
			++median_ptr;
		}
#endif
		// bump index
		if (++_idx == N){
			_idx = 0;
		}
		
		initialized = true;

		if (initialized) {
			// update outputs
			out[MEAN] = mean;
			out[MAX] = _max;
			out[MIN] = _min;
			if (calc_var) {
				out[VAR] = var;
				out[STDDEV] = sqrt(var);
			}
			if (calc_gradient) {
				out[GRADIENT] = b;
			}
			out[ZERO_CROSSING] = ((SIGNAL_T)zc) / N; // zero crossing as percentage of buffer size
			out[ENERGY] = sum_sqrs;
			out[POWER] = sum_sqrs / N;
		}

	}
};
// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="Stats">
	<SUMMARY>
Performs optimized running statistics on the signal.<br />
	</SUMMARY>
	<INPIN DEFAULT="true" />
	<OUTPIN NAME="MEAN"><SUMMARY>Moving average.</SUMMARY></OUTPIN>
	<OUTPIN NAME="MAX"><SUMMARY>Maximum value.</SUMMARY></OUTPIN>
	<OUTPIN NAME="MIN"><SUMMARY>Minimum value.</SUMMARY></OUTPIN>
	<OUTPIN NAME="VAR"><SUMMARY>Variance.</SUMMARY></OUTPIN>
	<OUTPIN NAME="STDDEV"><SUMMARY>StdDev (RMS).</SUMMARY></OUTPIN>
	<OUTPIN NAME="GRADIENT"><SUMMARY>The linear regression line's gradient.</SUMMARY></OUTPIN>
<CREATE_ARG NAME="size" MANDATORY="true" TYPE="Positive Integer">
	<SUMMARY>The size of the buffer in samples.</SUMMARY>
</CREATE_ARG>
 <ADJUST_ARG NAME="size" />
 <ADJUST_ARG NAME="interval">
	<SUMMARY>Adjusts the size of the buffer, but instead of specifying number of samples, the size is specified as a number of seconds.<br />
	So <PRE>interval == size / sample_rate</PRE></SUMMARY>
</ADJUST_ARG>

</PROC>
*/
struct sp_stats_FACTORY : public FactoryClass1Arg<sp_stats, int > 
{ 
	sp_stats_FACTORY() : 
		FactoryClass1Arg<sp_stats, int >("size")
	{
	} 
};
