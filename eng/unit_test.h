#pragma once
#include <iostream>
#include <exception>
#include <vector>
#if defined(COMPILE_UNIT_TESTS)
#ifndef COMPILE_WITH_PYTHON
#error COMPILE_WITH_PYTHON must be defined for unit tests
#endif

// Access to python libs for unit tests
#include "python.h"
// Unit tests  need to be instantiated

#define SEL_RUN_UNIT_TEST(TEST_NAME) TEST_NAME ## _ut::test TEST_NAME ## _ut_instance;

#define SEL_UNIT_TEST(TEST_NAME) \
namespace  TEST_NAME ## _ut { \
	struct test \
	{ \
		int npassed = 0; \
		int nfailed = 0; \
		test() \
		{ \
			std::cout << "UNIT TEST: " << #TEST_NAME << ": "; \
			try { \
			run(); \
			} catch(std::exception &ex) { std::cout << ex.what(); } \
			printf("Passed: %d Failed: %d\n\n", npassed, nfailed); \
		}

#define SEL_UNIT_TEST_END \
	} \
	; \
} // unit_test
void SEL_UNIT_TEST_ITEM(const char* msg) {
	printf("\n* SUBTEST: %s\n", msg);
}

#define SEL_UNIT_TEST_ASSERT(EXPR) if (EXPR) { ++npassed; } else { printf("\n\n>>>>>>>>>> ASSERT FAILED: (%s:%d) %s\n\n", __FILE__, __LINE__, #EXPR); ++nfailed; }

#define SEL_UNIT_TEST_EQUAL_THRESH(EXPR1, EXPR2, THRESH) \
	{ \
		auto a = EXPR1; auto b = EXPR2; \
			if (abs(a - b) < THRESH) { \
				++npassed; \
			} else { \
				printf("\n\n>>>>>>>>>> ASSERT FAILED: (%s:%d) %s (%f) !~ %s (%f)\n\n", __FILE__, __LINE__, #EXPR1, a, #EXPR2,  b); \
				++nfailed; \
			} \
	}

#define SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(EXPR1, EXPR2) SEL_UNIT_TEST_EQUAL_THRESH(EXPR1, EXPR2, 1e-7)

#else
	#define SEL_RUN_UNIT_TEST ((void *()) 0)
	#define SEL_UNIT_TEST  ((void *()) 0)
	#define SEL_UNIT_TEST_END  ((void *()) 0)
	#define SEL_UNIT_TEST_ASSERT ((void *()) 0)
	#define SEL_UNIT_TEST_EQUAL_THRESH ((void *()) 0)
	#define SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL ((void *()) 0)
#endif