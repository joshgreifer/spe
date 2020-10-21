// eng6_tests.cpp : Unit tests
//
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SCL_SECURE_NO_WARNINGS
	#define _SCL_SECURE_NO_WARNINGS
#endif
#endif

#define COMPILE_UNIT_TESTS

#include  "../eng/eng6.h"
#include  "../eng/unit_test.h"

//#include "../eng7/new_processor.h"
//
//struct testproc : sel::eng7::stdproc<10, 10> {
//	void process() {
//		this->out()[0] += 11.0;
//	}
//};
//struct testsource : sel::eng7::stdsource<10> {
//	testsource() {
//		for (auto& v : this->out())
//			v = 5.0;
//	}
//	void process() {
//		++this->out()[0];
//	}
//};
//
//struct testsink : sel::eng7::stdsink<10> {
//
//	void process() {
//		std::cout << "[ ";
//		for (auto v : this->in_v())
//			std::cout << v << ' ';
//		std::cout << " ]\n";
//	}
//};
//testsource p;
//testproc q;
//testsink r;

int main()
{
	//p.connect_to(q);
	//q.connect_to(r); // should fail to compile
	//p.process();
	//q.process();
	//r.process();
	////r.connect_to(p);  // should fail to compile

	//SEL_RUN_UNIT_TEST(dct)
	SEL_RUN_UNIT_TEST(numpy)
	//SEL_RUN_UNIT_TEST(fft)
	//SEL_RUN_UNIT_TEST(lattice_filter)
	// SEL_RUN_UNIT_TEST(periodic_event)
	//SEL_RUN_UNIT_TEST(resampler)
	//SEL_RUN_UNIT_TEST(window)
	//SEL_RUN_UNIT_TEST(quick_queue)
	//SEL_RUN_UNIT_TEST(rand)
	//SEL_RUN_UNIT_TEST(fir_filt)
	//SEL_RUN_UNIT_TEST(iir_filt)
	//SEL_RUN_UNIT_TEST(dnn)
	//SEL_RUN_UNIT_TEST(ewma)
	//SEL_RUN_UNIT_TEST(expr)
	//SEL_RUN_UNIT_TEST(lpc)
	SEL_RUN_UNIT_TEST(melspec)
	//SEL_RUN_UNIT_TEST(mux_demux)
	//SEL_RUN_UNIT_TEST(psd)
	/// TODO:  mag unit test
	//SEL_RUN_UNIT_TEST(mag)
	//SEL_RUN_UNIT_TEST(running_stats)

	return 0;
}

