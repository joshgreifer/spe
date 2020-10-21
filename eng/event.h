#pragma once
#include "object.h"
#include "eng_traits.h"
#include "rate_measure.h"

#include <iostream> // for trace
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

namespace sel {
	namespace eng {
		template<typename COUNTER_TYPE>class semaphore_t: public traceable<semaphore_t<COUNTER_TYPE>>
		{


		protected:

			void set_rate(rate_t new_rate) { rate_ = new_rate;  }
			void set_rate(size_t numer, size_t denom) { rate_ = rate_t(numer, denom);  }

			COUNTER_TYPE count() const { return _count;  }

			mutable bool enabled = true;

		private:

			mutable COUNTER_TYPE _count;
			rate_measure<COUNTER_TYPE> rate_;

		public:

			std::ostream& trace(std::ostream& os) const override
			{
				os << _count; //rate_.iters();
				return os;
			}


			rate_measure<COUNTER_TYPE>& rate() { return rate_; }

			size_t acquire() const
			{
				if (_count && enabled) {

					rate_.iterate(); // iterate the rate measurement when semaphore is acquired

					return _count--;
				}
				return 0;
			}
			//const rate_t& rate_;
			//const rate_t rate() const final { return rate_; }
			semaphore_t(size_t semaphore_count = 0, rate_t expected_rate = rate_t()) : rate_(expected_rate)
			{
				reset();
				_count = semaphore_count;
			}

			void raise(size_t semaphore_count = 1) const
			{
				_count += semaphore_count;
			}
			void reset() const
			{
				_count = 0;
			}
		};
		// For multi-threaded
		// typedef semaphore_t<std::atomic_size_t> semaphore;
		// For single-threaded
		typedef semaphore_t<std::size_t> semaphore;



	} // eng
} // sel


