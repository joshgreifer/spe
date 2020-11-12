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

#define COMPILE_WITH_PYTHON
#define COMPILE_UNIT_TESTS
#include  "../eng6/eng6.h"
#include  "../eng6/unit_test.h"



int main()
{

    SEL_UNIT_TEST_SUITE_BEGIN
	SEL_RUN_UNIT_TEST(dct)
	SEL_RUN_UNIT_TEST(fft)
	SEL_RUN_UNIT_TEST(lattice_filter)
	SEL_RUN_UNIT_TEST(periodic_event)
	SEL_RUN_UNIT_TEST(resampler)
	SEL_RUN_UNIT_TEST(window)
	SEL_RUN_UNIT_TEST(quick_queue)
	SEL_RUN_UNIT_TEST(rand)
	SEL_RUN_UNIT_TEST(fir_filt)
	SEL_RUN_UNIT_TEST(iir_filt)
	SEL_RUN_UNIT_TEST(dnn)
	SEL_RUN_UNIT_TEST(ewma)
	SEL_RUN_UNIT_TEST(expr)
	SEL_RUN_UNIT_TEST(lpc)
	SEL_RUN_UNIT_TEST(melspec)
//
//    SEL_RUN_UNIT_TEST(numpy)
//    SEL_RUN_UNIT_TEST(wavfile)
    SEL_RUN_UNIT_TEST(wav_file_reader)
//	SEL_RUN_UNIT_TEST(mux_demux)
//	SEL_RUN_UNIT_TEST(psd)
//	/// TODO:  mag unit test
//	//SEL_RUN_UNIT_TEST(mag)
//	SEL_RUN_UNIT_TEST(running_stats)

    SEL_UNIT_TEST_SUITE_RUN
	return 0;
}

