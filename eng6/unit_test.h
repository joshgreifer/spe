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

namespace sel {

     struct unit_test {

        bool run_and_store_results() {

            std::cout << "UNIT TEST: " << name() << ": ";
            try {
                run();
            } catch(std::exception &ex) { std::cerr << ex.what(); ++nfailed; }
            if (nfailed)  {
                std::cerr << "FAILED.\n";
                return false;
            } else if (verbose) {
                if (nfailed == 0 && npassed == 0)
                    std::cout << "NO TESTS IMPLEMENTED.\n";
                else
                    printf("\nPASSED. (%d assertion%s)\n", npassed, npassed == 1 ? "" : "s");

            }  else
                std::cout << "\n";
            std::cout << "=====================================================\n\n";
            return true;
        }
        virtual void run() = 0;
        virtual const char *name() const = 0;
        int npassed = 0;
        int nfailed = 0;
        bool verbose;

        unit_test(bool verbose) : verbose(verbose) {}

        virtual ~unit_test() = default;
    };

    struct test_suite {
        std::vector<unit_test *>tests;
        void add_test(unit_test *test) { tests.push_back(test); }
        bool run_all() {
            bool passed_all = true;
            for (auto t : tests)
                passed_all &= t->run_and_store_results();
            return passed_all;
        }
    };

}

// Unit tests  need to be instantiated

#define SEL_UNIT_TEST_SUITE_BEGIN sel::test_suite suite = {}; bool verbose = true;

#define SEL_UNIT_TEST_SUITE_RUN { \
bool passed = suite.run_all(); \
if (!passed)                      \
std::cerr << "\n*** FAILED.";     \
else                              \
std::cerr << "\n*** PASSED."; }

#define SEL_RUN_UNIT_TEST(TEST_NAME) suite.add_test(new TEST_NAME ## _ut::test(verbose));

#define SEL_UNIT_TEST(TEST_NAME) \
namespace  TEST_NAME ## _ut { \
	struct test : sel::unit_test\
	{                               \
        const char *name() const final { return #TEST_NAME;  }                         \
		test(bool verbose = true) : sel::unit_test(verbose) {}

#define SEL_UNIT_TEST_END \
	}; \
} // namespace
void SEL_UNIT_TEST_ITEM(const char* msg) {
	printf(" %s...", msg);
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