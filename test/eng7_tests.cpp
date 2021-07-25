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


#include  "../eng6/unit_test.h"
#include "../eng7/new_processor.h"
#include "../eng7/procs/rand.h"
#include "../eng7/procs/fir_filt.h"
#include "../eng7/procs/fft.h"
#include "../eng7/procs/numpy_ut.h"
#include "../eng7/procs/window_ut.h"
int main()
{
	SEL_UNIT_TEST_SUITE_BEGIN

    SEL_RUN_UNIT_TEST(fft)
//    SEL_RUN_UNIT_TEST(rand7)
//	SEL_RUN_UNIT_TEST(fir_filt7)
//    SEL_RUN_UNIT_TEST(numpy7)
    SEL_RUN_UNIT_TEST(window7)
    SEL_UNIT_TEST_SUITE_RUN
	return 0;
}

