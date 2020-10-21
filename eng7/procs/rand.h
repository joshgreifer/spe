#pragma once
#include <random>
#include <algorithm>
#include "../new_processor.h"
/*
 * output is identical to Matlab rand()
 * See my answer to https://stackoverflow.com/questions/24199376/matlab-rand-and-c-rand/51989341#51989341
 *
 */
// TODO: Unit test for this proc
namespace sel
{
	namespace eng7
	{
		namespace proc 
		{
			template<size_t OutW>struct rand : public stdsource<OutW>
			{
				std::mt19937 rng;
				std::uniform_real_distribution<float> urd;
				void process() final
				{
//					std::uniform_real_distribution<float> urd(0, 1);

					for (auto& a: this->out()) {
						a = urd(rng);
						#pragma warning(suppress: 4834)
						rng(); // discard next number, to match up with Matlab's rng
					}
				}
				// std::mt19937::default_seed == 5489U which matches Matlab
				explicit rand(unsigned int seed = std::mt19937::default_seed) : urd(0,1)
				{
					rng.seed(seed /* = 5489U */);
				}

			};
		}
	}
}
#if defined(COMPILE_UNIT_TESTS)
#include "../../eng/unit_test.h"

SEL_UNIT_TEST(rand7)
struct ut_traits
{
	static constexpr size_t signal_length = 10;
};

std::array<samp_t, ut_traits::signal_length> matlab_results = { { 0.814723686393179,	0.905791937075619,	0.126986816293506,	0.913375856139019,	0.632359246225410,	0.0975404049994095,	0.278498218867048,	0.546881519204984,	0.957506835434298,	0.964888535199277 } };
void run()
{
	auto& py_np_random = python::get().np.attr("random");
	const auto seed = 5489U;
	sel::eng7::proc::rand<ut_traits::signal_length> rng(seed);
	rng.process();
	const samp_t* matlab_result = matlab_results.data();
	for (auto& v : rng.out())
	{
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(v, *matlab_result++)
	}
	py_np_random.attr("seed")(seed);
	py::array_t<float> rand_py = py_np_random.attr("random")(ut_traits::signal_length);
	auto rand_py_data = rand_py.data();
	for (size_t i = 0; i < ut_traits::signal_length; ++i)
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(rand_py_data[i], rng.out()[i]);
}
SEL_UNIT_TEST_END
#endif


