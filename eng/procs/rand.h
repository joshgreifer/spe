#pragma once
#include <random>
#include <algorithm>
#include "data_source.h"
/*
 * output is identical to Matlab rand()
 * See my answer to https://stackoverflow.com/questions/24199376/matlab-rand-and-c-rand/51989341#51989341
 *
 */
// TODO: Unit test for this proc
namespace sel
{
	namespace eng
	{
		namespace proc 
		{
			template<size_t OutW>struct rand : public sel::eng::proc::data_source<OutW>
			{
				std::mt19937 rng;
				std::uniform_real_distribution<float> urd;
				void process() final
				{
//					std::uniform_real_distribution<float> urd(0, 1);

					for (auto& a: this->oport) {
						a = urd(rng);
						#pragma warning(suppress: 4834)
						rng(); // discard next number, to match up with Matlab's rng
					}
				}
				explicit rand() : urd(0,1)
				{
					// rng.seed(std::mt19937::default_seed /* = 5489U */);
				}

			};
		}
	}
}
#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"

SEL_UNIT_TEST(rand)
struct ut_traits
{
	static constexpr size_t signal_length = 10;
};

std::array<samp_t, ut_traits::signal_length> matlab_results = { { 0.814723686393179,	0.905791937075619,	0.126986816293506,	0.913375856139019,	0.632359246225410,	0.0975404049994095,	0.278498218867048,	0.546881519204984,	0.957506835434298,	0.964888535199277 } };
void run()
{
	sel::eng::proc::rand<ut_traits::signal_length> rng;
	rng.process();
	const samp_t* matlab_result = matlab_results.data();
	for (auto& v : rng.oport)
	{
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(v, *matlab_result++)
	}
}
SEL_UNIT_TEST_END
#endif


