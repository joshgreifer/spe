#pragma once
#include "sp.h"
/*
FIR Direct Form Filter
*/



// http://zone.ni.com/reference/en-XX/help/371325F-01/lvdfdtconcepts/fir_filter_specs/

class sp_fir_filter : public SigProc, public iRunTimeModifiable
{
	size_t _sz;
	SIGNAL_T *_buf;
	double *h;
	int n;
	bool bypass;
public:
	const char* name() const { return "FIR Filter"; }
	PIN default_inpin() const { return PIN1; }
	PIN default_outpin() const { return PIN1; }


	sp_fir_filter(const vector<double>& coeffs )
		: n(0),
		_sz(coeffs.size()),
		_buf( new SIGNAL_T[coeffs.size()]),
		h( new double[coeffs.size()]),
		bypass(false),
		SigProc(1,1)
	{
		::memset(_buf, 0, _sz * sizeof(SIGNAL_T));
		for (size_t i = 0; i < _sz; ++i)
			h[i] =  coeffs[i];
	}

	virtual ~sp_fir_filter(void)
	{
		delete [] _buf;
		delete [] h;
	}

	void Modify(params& args) 
	{
		bypass = mand_arg<bool>(args, "bypass");
	}

	void process()
	{
		_buf[n] = *in[0];

		if (++n == _sz)
			n = 0;

		double output = 0.0;
		for (size_t i = 0; i < _sz; ++i)
			output += (h[i]) * _buf[(i + n) % _sz];

		if (bypass)
			out[0] = *in[0];
		else
			out[0] = (SIGNAL_T)output;
	}
};

// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="FIRFilt">
	<SUMMARY>
		FIR (Finite Impulse Response) filter.<br />
		The implementation is a <a href="http://cnx.org/content/m11918/latest/" >direct-form I filter</a>, with an array of coefficients: <br />
		The filter coefficients are usually obtained from an external program like Matlab&#39;s fdatool.
	</SUMMARY>
	<INPIN DEFAULT="true" />
	<OUTPIN DEFAULT="true" />

	<CREATE_ARG NAME="coeffs" MANDATORY="true" TYPE="double_vector">
		<SUMMARY>The coefficients.</SUMMARY>
	</CREATE_ARG>
	<ADJUST_ARG NAME="Bypass" TYPE="boolean">
		<SUMMARY>When true, the filter will be bypassed, i.e. its output will be identical to its input. However, the filter will continue to update its internal state.</SUMMARY>
	</ADJUST_ARG>

</PROC>
*/
struct sp_fir_filter_FACTORY : public FactoryClass1Arg<sp_fir_filter, vector<double> > 
{ 
	sp_fir_filter_FACTORY() : 
		FactoryClass1Arg<sp_fir_filter, vector<double> >("coeffs")
	{
	} 
};
