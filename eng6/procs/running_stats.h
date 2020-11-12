#pragma once
#include "../processor.h"
#include "../idx.h"
#include <vector>



namespace sel {
	namespace eng6 {
		namespace proc {


			template<size_t Sz /*, typename=std::enable_if<Sz!=0> */ >class running_stats : public  Processor<1, 9>, virtual public creatable<running_stats<Sz /*,  size_t */ >>
			{
				size_t idx_ = 0;
				vector<samp_t> buf_;
				samp_t sum_ = 0.0;
				samp_t sum_sqrs_ = 0.0; // Sum of squared values (for energy/power calc)
				//samp_t b2 = 0; // see linear regression.  These values are precalculated in ctor

				// recalculate running sum from scratch to prevent f.p. accuracy loss
				static constexpr size_t recalc_sum_iters = 100000;
				static constexpr samp_t mean_x() { return  (Sz-1) / 2.0; }

				static constexpr samp_t b2(const size_t i = Sz)
				{
					if (i == 0)
						return 0.0;
					return (i-mean_x()) * (i-mean_x()) + b2(i-1);
				}

				double sample_interval = 0.0;
				bool buffer_primed_ = false;

				bool calc_var_ = false;
				bool calc_gradient_ = false;

				size_t recalc_counter_ = 0;
				
			public:
					static constexpr size_t port_id_mean() { return 0; }
					static constexpr size_t port_id_var() { return 1; }
					static constexpr size_t port_id_stddev() { return 2; }
					static constexpr size_t port_id_max_in_range() { return 3; }
					static constexpr size_t port_id_min_in_range() { return 4; }
					static constexpr size_t port_id_gradient() { return 5; }
					static constexpr size_t port_id_zero_crossing() { return 6; }
					static constexpr size_t port_id_energy() { return 7; }
					static constexpr size_t port_id_power() { return 8; }

				explicit running_stats() : buf_(Sz) {}

				explicit running_stats(params& args)
				{
					

				}

				virtual const std::string type() const final { return "running stats"; }

				void init(schedule *context) final
				{

					// stats optimization flags
					calc_var_ = is_output_connected(port_id_var()) || is_output_connected(port_id_stddev());
					calc_gradient_ = is_output_connected(port_id_gradient());

					sample_interval = context->expected_rate().recip();
				}

				void process() final 
				{
					const auto v = *this->inports[0]->as_array();
					
					const auto oldest_v = buf_[idx_];

					sum_ -= oldest_v;
					sum_sqrs_ -= oldest_v * oldest_v;
					buf_[idx_] = v; 

					// update sum
					if (++recalc_counter_ >= recalc_sum_iters) {
						recalc_counter_ = 0;
						sum_ = sum_sqrs_ = 0.0;
						for (size_t i = 0; i < Sz; ++i) {
							const auto v2 = buf_[i];
							sum_ += v2;
							sum_sqrs_ += v2 * v2;
						}
					} else {
						sum_ += v;
						sum_sqrs_ += v * v;
					}
					const auto n = buffer_primed_ ? Sz : idx_+1;

					auto mean = sum_ / n;

					// TODO: Implement better algorithm See http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance

					auto max = -DBL_MAX, min = DBL_MAX;
					auto var = 0.0;
					
					auto b1 = 0.0;

					size_t zc = 0;

					for (size_t i = 0; i < n; ++i) {

						const auto vv = buf_[i];
						// zero crossing
						if (i > 0)
							if ((vv > 0.0) != (buf_[i - 1] > 0.0))
								++zc;

						// linear regression
						if (calc_gradient_) {
							auto idx = idx_+i; if (idx >= n) idx -= n;
							b1 += (i - mean_x()) * (buf_[idx] - mean);
						}
						// var
						if (calc_var_) {
							const auto diffmean = vv - mean;
							const auto diffmean_squared = diffmean * diffmean;
							var += diffmean_squared;
						}
						// min, max
						if (vv > max)
							max = vv;
						if (vv < min)
							min  =vv;
					}
					// normalize var
					var /= (n-1);
					
					// bump index
					if (++idx_ == Sz) {
						idx_ = 0;
						buffer_primed_ = true;
					}

					// update outputs
					*outports[port_id_mean()]->as_array() = mean;
					*outports[port_id_max_in_range()]->as_array() = max;
					*outports[port_id_min_in_range()]->as_array() = min;
					if (calc_var_) {
						*outports[port_id_var()]->as_array() = var;
						*outports[port_id_stddev()]->as_array() = sqrt(var);
					}
					if (calc_gradient_) {
						*outports[port_id_gradient()]->as_array() = b1 / b2();
					}
					*outports[port_id_zero_crossing()]->as_array() = static_cast<samp_t>(zc) / Sz; // zero crossing as percentage of buffer size
					*outports[port_id_energy()]->as_array() = sum_sqrs_;
					*outports[port_id_power()]->as_array() = sum_sqrs_ / Sz;
					
				}
			};
		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "rand.h"
#include "../unit_test.h"

SEL_UNIT_TEST(running_stats)
struct ut_traits
{
	static constexpr size_t signal_length = 1000;
	static constexpr size_t stats_size = 1000;
};

using stats_t = sel::eng6::proc::running_stats<ut_traits::stats_size>;

std::map<size_t, samp_t> matlab_results = { 
	{ stats_t::port_id_mean(), 0.488832612865264 } 
};

void run()
{
	sel::eng6::proc::rand<1> rng;

	stats_t stats;
	rng.ConnectTo(stats);
	rng.freeze();
	stats.freeze();

	for (size_t iter = 0; iter < ut_traits::signal_length; ++iter)
	{
		rng.process();
		stats.process();

	}
	SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(stats.Out(stats_t::port_id_mean())->as_array()[0], matlab_results[stats_t::port_id_mean()])

}
SEL_UNIT_TEST_END
#endif




