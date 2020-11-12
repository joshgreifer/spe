#pragma once
#include "sp.h"
#include <math_defines.h>
/*
IIR Filter
*/

class sp_iir_filter : public SigProc, public iRunTimeModifiable
{
	size_t _sz;
	SIGNAL_T *_w;
	const double *_b;
	const double *_a;
	bool bypass;

public:
	const char* name() const { return "IIR Filter"; }
	PIN default_inpin() const { return PIN1; }
	PIN default_outpin() const { return PIN1; }


	sp_iir_filter(const vector<double>& b,  const vector<double>& a)
		: _sz(b.size()),
		_w( new SIGNAL_T[b.size()] ),
		_b( new double[b.size()] ),
		_a( new double[a.size()] ),
		bypass(false),
		SigProc(1,1)
	{
		if (b.size() != a.size())
			throw std::exception("IIR Filter: numerator (b) and denominator (a) coefficent vectors differ in size.");
		for (size_t i = 0; i < _sz; ++i) {
			_w[i] = (SIGNAL_T)0.0;
			const_cast<double *>(_b)[i] =  b[i];
			const_cast<double *>(_a)[i] =  a[i];
		}
		// guard against divide by zero in filter
		if (_a[0] == 0.0)
			throw std::exception("IIR Filter: first denominator (a) coefficent cannot be zero.");
	}

	virtual ~sp_iir_filter(void)
	{
		delete [] _w;
		delete [] _b;
		delete [] _a;
	}

	void Modify(params& args) 
	{
		bypass = mand_arg<bool>(args, "bypass");
	}

	void process()
	{
		size_t i;
		double y = 0;

		_w[0] = *in[0];						// current input sample

		for ( i = 1; i < _sz; ++i )			// input adder
			_w[0] -= (SIGNAL_T)(_a[i] * _w[i]);

		for ( i = 0; i < _sz ; ++i )		// output adder
			y += _b[i] * _w[i];

		// now i == K
		for (--i; i > 0; --i )				// shift buf backwards
			_w[i] = _w[i-1];
		
		if (bypass)
			out[0] = *in[0];
		else
			out[0] =  (SIGNAL_T)(y / _a[0]);	// current output sample
	}
};


// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="IIRFilt">
	<SUMMARY>
		IIR (Infinite Impulse Response) filter.<br />
		The implementation is a <a href="http://cnx.org/content/m11919/latest/" >direct-form I filter</a>, with two arrays of coefficients: <br />
		The &quot;B&quot; coefficents are the numerators, <br />
		The &quot;A&quot; coefficents are the denominators.  The first &quot;A&quot; coefficient cannot be zero. <br />
		The filter coefficients are usually obtained from an external program like Matlab&#39;s fdatool.
	</SUMMARY>
	<INPIN DEFAULT="true" />
	<OUTPIN DEFAULT="true" />

	<CREATE_ARG NAME="B" MANDATORY="true" TYPE="double_vector">
		<SUMMARY>The numerator coefficients.</SUMMARY>
	</CREATE_ARG>
	<CREATE_ARG NAME="A" MANDATORY="true" TYPE="double_vector">
		<SUMMARY>The denominator coefficients.</SUMMARY>
	</CREATE_ARG>
	<ADJUST_ARG NAME="Bypass" TYPE="boolean">
		<SUMMARY>When true, the filter will be bypassed, i.e. its output will be identical to its input. However, the filter will continue to update its internal state.</SUMMARY>
	</ADJUST_ARG>

</PROC>
*/
struct sp_iir_filter_FACTORY : public FactoryClass2Args<sp_iir_filter, vector<double>, vector<double> > 
{ 
	sp_iir_filter_FACTORY() : 
		FactoryClass2Args<sp_iir_filter, vector<double>, vector<double> >("b", "a")
	{
	} 
};

// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="LPFilt">
	<SUMMARY>
		6 dB per octave low pass (RC) filter.  This is implemented as an IIR filter, where the A and B coefficents are dervied thus:<br />
		<code>
		K = 1-exp(2PI Fc/Fs), <br />
		b = [ K/2 K/2 ], <br />
		a = [ 1 K-1 ].
		</code>
		<br />
		<code>Fs</code> is the sampling frequency, and <code>Fc</code> is the cuttoff frequency.<br />
		See Hal Chamberlin's &quot;Musical Applications of Microprocssors&quot; for a discussion and explanation.
		</SUMMARY>
	<INPIN DEFAULT="true" />
	<OUTPIN DEFAULT="true" />

	<CREATE_ARG NAME="cutoff" MANDATORY="true" TYPE="double">
		<SUMMARY>Fc/Fc, as explained above.  So a better name for this parameter should be &quot;cutoff_over_sampling_rate&quot;.</SUMMARY>
	</CREATE_ARG>
	<ADJUST_ARG NAME="Bypass" TYPE="boolean">
		<SUMMARY>When true, the filter will be bypassed, i.e. its output will be identical to its input. However, the filter will continue to update its internal state.</SUMMARY>
	</ADJUST_ARG>

</PROC>
*/
struct sp_lp_filter_FACTORY : public iFactoryClass
{
	std::string TypeName() const { return "lpfilter"; }

	iCreatable *Create(params& args) const
	{
		double fc_over_fs = mand_arg<double>(args, "cutoff");
		if (fc_over_fs <= 0.0 || fc_over_fs > 1.0)
			throw eng_ex("Illegal argument value, must be between 0.0 and 1.0");
		double K = 1 - exp(-2 * PI * fc_over_fs );
		vector<double>b;
		vector<double>a;
		b.push_back( K / 2.0 );
		b.push_back( K / 2.0 );
		a.push_back( 1.0 );
		a.push_back( K - 1.0 );

		return new sp_iir_filter(b, a);
	}
};

// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="HPFilt">
	<SUMMARY>
		6 dB per octave high pass (RC) filter.  This is implemented as an IIR filter, where the A and B coefficents are dervied thus:<br />
		<code>
		K = 1-exp(2PI Fc/Fs), <br />
		b = [ 1-K/2 K/2-1 ], <br />
		a = [ 1 K-1 ].
		</code>
		<br />
		<code>Fs</code> is the sampling frequency, and <code>Fc</code> is the cuttoff frequency.<br />
		See Hal Chamberlin's &quot;Musical Applications of Microprocssors&quot; for a discussion and explanation.
		</SUMMARY>
	<INPIN DEFAULT="true" />
	<OUTPIN DEFAULT="true" />

	<CREATE_ARG NAME="cutoff" MANDATORY="true" TYPE="double">
		<SUMMARY>Fc/Fc, as explained above.  So a better name for this parameter should be &quot;cutoff_over_sampling_rate&quot;.</SUMMARY>
	</CREATE_ARG>
	<ADJUST_ARG NAME="Bypass" TYPE="boolean">
		<SUMMARY>When true, the filter will be bypassed, i.e. its output will be identical to its input. However, the filter will continue to update its internal state.</SUMMARY>
	</ADJUST_ARG>

</PROC>
*/struct sp_hp_filter_FACTORY : public iFactoryClass
{
	std::string TypeName() const { return "hpfilter"; }

	iCreatable *Create(params& args) const
	{
		double fc_over_fs = mand_arg<double>(args, "cutoff");
		if (fc_over_fs <= 0.0 || fc_over_fs > 1.0)
			throw eng_ex("Illegal argument value, must be between 0.0 and 1.0");
		double K = 1 - exp(-2 * PI * fc_over_fs );
		vector<double>b;
		vector<double>a;
		b.push_back( 1.0 - K / 2.0 );
		b.push_back( K / 2.0 - 1.0 );
		a.push_back( 1.0 );
		a.push_back( K - 1.0 );

		return new sp_iir_filter(b, a);
	}
};
