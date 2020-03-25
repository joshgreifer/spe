#pragma once

#include <IDX.h>
#include "sp.h"
/*
Lattice MA and AR filters 

*/



// http://zone.ni.com/reference/en-XX/help/371325F-01/lvdfdtconcepts/lattice_ma_specs/

template<size_t NUM_COEFFS>class sp_lattice_filter : public RegisterableSigProc<sp_lattice_filter<NUM_COEFFS>, 1, 1>, public iRunTimeModifiable
{
	static constexpr size_t M = NUM_COEFFS + 1;
	// Internal state 
	SIGNAL_T state[M];

	// array of coefficients.
	// Set (in constructor or via Modify() ) to either an array of literal values, or to the output pins of another proc with NUM_COEFFS output pins
	const SIGNAL_T *coeffs_;  
	SIGNAL_T k[M]; // populated from above, but prefixed by 1.0 to help the loop code

	bool bypass;
	params *init_args;

	// pointer to step() function for the differnt flavours of lattice filter (Currently MA and AR)
	SIGNAL_T (sp_lattice_filter::*step)(SIGNAL_T) = &sp_lattice_filter::step_moving_average;

	// MA step()
	SIGNAL_T step_moving_average(SIGNAL_T x)
	{
		double y = x;
		double s[M]; // work array

					 //  Save current states before we overwrite them
		s[0] = 0.0;
		for (size_t m = 1; m < M; ++m)
			s[m] = state[m - 1];


		for (size_t m = 0; m < M; ++m) {
			double ty = y;
			y += s[m] * k[m]; // this line does nothing at m==0, because s[0] is zero. But helps loopify the code
			state[m] = s[m] + k[m] * ty;
		}
		return y;

	}

	// AR step() 
	SIGNAL_T step_autoregressive(SIGNAL_T x)
	{

		double y = x;


		for (size_t i = 0; i < M - 1; ++i) {
			size_t m = M - i - 1; // index into k

			y -= k[m] * state[i + 1];
			state[i] = state[i + 1] + y * k[m];


		}
		state[M - 1] = y;

		return y;

	}
public:
	const char* name() const { return "Lattice MA Filter"; }
	PIN default_inpin() const { return ALL; }
	PIN default_outpin() const { return PIN1; }


	sp_lattice_filter() : init_args(nullptr) {}

	sp_lattice_filter(params& args)
		: init_args(args.clone())
	{
		auto type = mand_arg<std::string>(args, "type");
		if (!_stricmp(type.c_str(), "MA"))
			step = &sp_lattice_filter::step_moving_average;
		else if (!_stricmp(type.c_str(), "AR"))
			step = &sp_lattice_filter::step_autoregressive;
		else
			throw eng_ex(EngineFormatMessage("Unknown lattice filter type: Types are MA, AR\n"));

	}

	// Mainly for unit testing - create coeffs from ConnectableArray
	sp_lattice_filter(ConnectableArray<SIGNAL_T, NUM_COEFFS>& coeffs) : init_args(nullptr)
	{
		coeffs_ = coeffs.Out(PIN1);
	}


	virtual ~sp_lattice_filter(void)
	{
		delete init_args;
	}

	void Modify(params& args)
	{
		set_arg(args, "bypass", bypass);


		const std::vector<double>& coeffs = mand_arg<std::vector<double> >(args, "reflection_coefficents");
		if (coeffs.size() == 0) {
			const char *k_sp_id = get_arg(args, "reflection_coefficents");
			auto s = std::string(k_sp_id);
			auto src_proc = InstanceManager::instance()->GetProc(s); // throws exception if not found/not a sigproc
			src_proc->Out((PIN)(NUM_COEFFS - 1));  // throws exception if src_proc has fewer than NUM_COEFFS outputs;
			coeffs_ = src_proc->Out(PIN1);
			

		}
		else {
			if (coeffs.size() != NUM_COEFFS)
				throw eng_ex(EngineFormatMessage("Incorrect number of reflection coefficients, expected: %zd, got: %zd", NUM_COEFFS, coeffs.size()));
			coeffs_ = coeffs.data();

		}

		// make coeffs 1-indexed, with k[0] == 1.0;
		k[0] = 1.0;
		for (size_t i = 0; i < NUM_COEFFS; ++i) 
			k[i + 1] = coeffs_[i];
		
	
	}

	void init(Schedule *context)
	{
		::memset(state, 0, M * sizeof(SIGNAL_T));

		if (init_args)
			Modify(*init_args);


	}



	void process()
	{

		double x = *this->in[0];
		double y = (this->*step)(x);
	
		if (bypass)
			this->out[0] = x;
		else
			this->out[0] = y;
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

