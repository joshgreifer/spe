#pragma once
#include <iostream>


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
			run(); \
			printf("Passed: %d Failed: %d\n\n", npassed, nfailed); \
		}

#define SEL_UNIT_TEST_END \
	} \
	; \
} // unit_test

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

