#pragma once

// mux_demux and demux unit tests
#include "mux_demux.h"
#include "samples.h"
#include "../unit_test.h"

SEL_UNIT_TEST(mux_demux)


struct ut_traits
{
	static constexpr size_t input_size = 19;
	static constexpr size_t output_frame_size = 7;
	static constexpr size_t iters = output_frame_size * 7;

};
using mux_demux = sel::eng::proc::mux_demux <ut_traits::input_size, ut_traits::output_frame_size>;


struct sig_gen_ramp : sel::eng::Processor01A<ut_traits::input_size>
{
	size_t c = 0;
	void process() final
	{
		for (size_t i = 0; i < ut_traits::input_size; ++i)
			out[i] = static_cast<samp_t>(c++);
	}
};


void run() {

	mux_demux mux_demux;
	sig_gen_ramp sig_gen_ramp;
	// sel::eng::proc::sample::Logger logger;
	struct null_sink : sel::eng::Processor1A0<ut_traits::output_frame_size>
	{
		void process() final {}
	} logger;

	sel::eng::proc::compound_processor input_proc;
	sel::eng::proc::compound_processor output_proc;


	input_proc.connect_procs(sig_gen_ramp, mux_demux.input());
	output_proc.connect_procs(mux_demux.output(), logger);


	sel::eng::semaphore sem;

	sel::eng::schedule input_schedule(&sem, input_proc);
	sel::eng::schedule output_schedule(&mux_demux, output_proc);

//			input_schedule.init();
//			output_schedule.init();

	// try invalid triggers
	sel::eng::schedule illegal_input_schedule(&mux_demux, input_proc);
	sel::eng::schedule illegal_output_schedule(&sem, output_proc);

	bool threw_exception = false;
	try {
		illegal_input_schedule.init();

	}
	catch (sel::eng_ex&) {
		// expect this
		threw_exception = true;
	}
	SEL_UNIT_TEST_ASSERT(threw_exception);

	threw_exception = false;
	try {
		illegal_output_schedule.init();

	}
	catch (sel::eng_ex&) {
		// expect this
		threw_exception = true;
	}
	SEL_UNIT_TEST_ASSERT(threw_exception);

	sel::eng::scheduler s = {};

	sem.raise(ut_traits::iters);
	s.add(input_schedule);
	s.add(output_schedule);
	s.init();
	while (s.step())
		;

	const samp_t *results = logger.in_as_array(0);

	size_t expected = ut_traits::input_size * ut_traits::iters - ut_traits::output_frame_size;
	// only works if iter is a multiple of output_size, otherwise mux_demux output will be in an intermidate state (i.e. it hasn't just triggered)
	for (size_t i = 0; i < ut_traits::output_frame_size; ++i)
		SEL_UNIT_TEST_ASSERT(results[i] == expected++);


}

SEL_UNIT_TEST_END
