#pragma once

#include <chrono>


namespace sel {
	template<class counter_t = size_t>class rate_measure
	{
		mutable rate_t expected_rate_;
		mutable counter_t _iters;
		mutable counter_t _last_measured_iters;
		mutable double _sampling_period_start;


		using clock = std::chrono::high_resolution_clock;

		double now() const { return (double)clock::now().time_since_epoch().count() / 1000000000.0; }
	public:

		rate_measure & operator=(const rate_measure&& other) { return *this; }
		rate_measure & operator=(const rate_t& expected_rate) {
			this->expected_rate_ = expected_rate;
			return *this;
		}

		rate_measure(rate_t expected_rate = rate_t()) :
			expected_rate_(expected_rate),
			_iters(0), _last_measured_iters(0) {}

		rate_t expected() const { return expected_rate_; }

		auto iters() const { return _iters; }

		double actual() const {


			double r = (_iters - _last_measured_iters) / (now() - _sampling_period_start);

			return r;
		}

		void begin_sampling() const
		{
			_last_measured_iters = _iters;
			_sampling_period_start = now();
		}

		void iterate() const {
			++_iters;
			//if (_iters++ == 0)
			//	begin_sampling();
		}

	};
} // sel