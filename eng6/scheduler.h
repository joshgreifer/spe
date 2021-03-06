#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED
#include <thread>
#include <atomic>
#include "singleton.h"
#include "processor.h"
#include "event.h"
#include <boost/asio.hpp>
#include <iostream> // for trace

#define USE_ASIO
/*  Event scheduling */
namespace sel {
	namespace eng6 {
		class asio_scheduler : public singleton<asio_scheduler>
		{
			using io_context = boost::asio::io_context;
			static singleton<io_context> context;
		public:
			void stop()
			{
				context.get().restart();
			}
			asio_scheduler()
			{
				context.get();
			}

			void queue_work_item(func action_) {
				io_context& ioc = context.get();
				boost::asio::post(ioc, action_);
			}

			// Returns the number of callbacks that were run
			size_t poll()
			{
				io_context& ioc = context.get();
				return ioc.poll();

			}
			// Returns the number of callbacks that were run
			size_t poll_one()
			{
				io_context& ioc = context.get();
				return ioc.poll_one();

			}


			boost::asio::io_context& ioc() { return context.get(); }
		};


		
		class periodic_event : public semaphore, public creatable<periodic_event>
		{
			std::chrono::nanoseconds period_ns_;
			boost::asio::high_resolution_timer timer_;


			void reschedule()
			{
				//					std::cerr << "\n> Reschedule:          Semaphore count " << this->trigger_ << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
				timer_.async_wait([this](const boost::system::error_code& e) {

					if (e) {

						std::cerr << "\n*** Periodic event cancelled: " << e.message();

					}
					else {
						//reset();
						this->raise();
						//std::cerr << "\n< Reschedule callback: Semaphore count " << this->count() << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
						timer_.expires_at(timer_.expiry() + period_ns_);
						reschedule();
					}
					});


			}
		public:
			periodic_event() : period_ns_(0), timer_(asio_scheduler::get().ioc()) {}

			periodic_event(params& args) : periodic_event(args.get<rate_t>("rate")) {}

			periodic_event(const rate_t& rate) : semaphore(0, rate),
				period_ns_(std::chrono::nanoseconds(static_cast<long long>(1000000000.0 / rate))),
				timer_(asio_scheduler::get().ioc())
			{
			}

			void init()
			{
				timer_.expires_after(period_ns_);
				reschedule();

			}
			virtual ~periodic_event()
			{
				timer_.cancel();
			}

		};
//		class periodic_event_orig : public semaphore, public creatable<periodic_event>
//		{
//			std::chrono::nanoseconds period_ns_;
//			boost::asio::high_resolution_timer timer_;
//
//			void reschedule()
//			{
////					std::cerr << "\n> Reschedule:          Semaphore count " << this->trigger_ << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
//				timer_.async_wait([this](const boost::system::error_code& /*e*/) {
//					reset();
//					raise();
//					//std::cerr << "\n< Reschedule callback: Semaphore count " << this->count() << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
//					timer_.expires_at(timer_.expiry() + period_ns_);
//					reschedule();
//
//				});
//				
//			}
//		public:
//			periodic_event_orig() : period_ns_(0), timer_(asio_scheduler::get().ioc()) {}
//			
//			periodic_event_orig(params& args) : periodic_event_orig(args.get<rate_t>("rate")) {}
//			
//			periodic_event_orig(const rate_t& rate) : semaphore(0, rate),
//				period_ns_(std::chrono::nanoseconds(static_cast<long long>(1000000000.0 / rate))),
//				timer_(asio_scheduler::get().ioc())
//			{
//				timer_.expires_after(period_ns_);
//				reschedule();
//			}
//
//
//
//		};
		
//		class periodic_func : public schedule
//		{
//			semaphore trigger_;
//			boost::asio::chrono::nanoseconds period_ns_;
//			boost::asio::high_resolution_timer timer_;
//
//			void reschedule()
//			{
////					std::cerr << "\n> Reschedule:          Semaphore count " << this->trigger_ << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
//				timer_.async_wait([this](const boost::system::error_code& /*e*/) {
//					this->trigger_.reset();
//					this->trigger_.raise();
////					std::cerr << "\n< Reschedule callback: Semaphore count " << this->trigger_ << "\tasio_timer expiry " << std::chrono::time_point_cast<std::chrono::seconds>(timer_.expiry()).time_since_epoch().count();
//					timer_.expires_at(timer_.expiry() + period_ns_);
//					reschedule();
//
//				});
//				
//			}
//		public:
//			periodic_func(const rate_t& rate, functor& action) :
//				schedule(&trigger_, action),
//				trigger_(0, rate),
//				period_ns_(boost::asio::chrono::nanoseconds(static_cast<long long>(1000000000.0 / rate))),
//				timer_(asio_scheduler::get().ioc())
//			{
//				timer_.expires_after(period_ns_);
//				reschedule();
//			}
//
//		};
		class schedule final : public traceable<schedule>
		{
			friend class scheduler;

			semaphore* trigger_;
			function_object f_;
			functor& action_;

		public:
			auto trigger() const { return trigger_; }
			auto& action() const { return action_; }

			virtual std::ostream& trace(std::ostream& os) const override

			{
				os << "{ " << *trigger_ << ": " << action_ << " }";
				return os;
			}


			schedule(semaphore* trigger, functor& action_) :
				trigger_(trigger),
				action_(action_) {}

			schedule(semaphore* trigger_, func f) :
				trigger_(trigger_),
				f_(function_object(f)),
				action_(f_) {}

			size_t acquire() const { return trigger_->acquire(); }

			void invoke(size_t repeat_count = 1U) {
				trigger_->raise(repeat_count);
				while (trigger_->acquire())
					action_();

			}

			// freeze and init the action if it's a processor
			virtual void init()
			{
				if (auto* pt = dynamic_cast<periodic_event*>(trigger_))
					pt->init();

				auto c = dynamic_cast<freezeable*>(&action_);
				if (c) c->freeze();

				auto p = dynamic_cast<processor*>(&action_);

				if (p) p->init(this);

			}
			virtual void term()
			{
				auto p = dynamic_cast<processor*>(&action_);

				if (p) p->term(this);
			}

			void begin_sampling() const { trigger_->rate().begin_sampling(); }
			double actual_rate() const { return trigger_->rate().actual(); }
			rate_t expected_rate() const { return trigger_->rate().expected(); }

		};
		class scheduler : public singleton<scheduler>
		{

		private:

			std::atomic_bool stop_request;

			std::vector<schedule> schedules;

			schedule *current_context_ = nullptr;


			// Returns the number of callbacks that were run
			size_t service_all_pending_aio()
			{
#ifdef USE_ASIO
				return asio_scheduler::get().poll();
#else
#ifdef _WIN32
				for (size_t i = 0; ; ++i)
					if (::SleepEx(0, TRUE) != WAIT_IO_COMPLETION)
						return i;
				return 0;
#endif
#endif
			}

			size_t service_any_pending_aio()
			{
#ifdef USE_ASIO
				return asio_scheduler::get().poll_one();
#else
#ifdef _WIN32
				
				if (::SleepEx(0, TRUE) != WAIT_IO_COMPLETION)
					return 1;
				return 0;
#endif
#endif
			}
		public:
			std::atomic_bool do_measure_performance_at_start;

			void start_performance_measure() const
			{
				for (const auto& s : schedules)
					s.begin_sampling();
			}
			
			scheduler() = default;

			static void queue_work_item(func action) { asio_scheduler::get().queue_work_item(action); }

			void clear()
			{
				schedules.clear();
			}

			const schedule *context() const
			{
				return current_context_;
			}
			

			void add(schedule& s)
			{
				// find schedule with this trigger
				auto i = find_if(schedules.begin(), schedules.end(), [s](auto s2) -> bool { return s2.trigger_ == s.trigger_; });

				if (i == schedules.end()) {  // no schedule with this trigger found, add a new chain
					schedules.push_back(s);
				}
				else
					//if ((*i).action == action)
					//	throw eng_ex("Schedule already registered");
					//else
					throw eng_ex("Attempt to add a schedule with same trigger as a registered schedule");
			}
			
			void add(semaphore * trigger, functor& action_)
			{

				// find schedule with this trigger
				auto i = find_if(schedules.begin(), schedules.end(), [trigger](auto s2) -> bool { return s2.trigger_ == trigger; });

				if (i == schedules.end()) {  // no schedule with this trigger found, add a new chain
					schedules.push_back(schedule(trigger, action_));
				}
				else
					//if ((*i).action == action)
					//	throw eng_ex("Schedule already registered");
					//else
					throw eng_ex("Attempt to add a schedule with same trigger as a registered schedule");
			}

			std::thread start()
			{
				if (schedules.size() == 0)
					throw eng_ex("There are no schedules to run.");


				return std::thread([=] { run(); });
			}

			void stop() { stop_request = true; }

			void init()
			{
				// If any schedule actions are connectables, run their freeze() routines
				for (auto &s : schedules) 
					//			cout << "* Schedule op target type : " << s.action.op.target_type().name() << endl;
					s.init();
					
				
			}

			size_t step()
			{
				size_t n_actions_run = 0;

				// Run all ready schedules;
				for (auto& s : schedules) {
					if (s.acquire()) {
						current_context_ = &s;
						s.action_();
						++n_actions_run;
					}
				}
				return n_actions_run;
			}

			void run()
			{
				if (schedules.size() == 0)
					throw eng_ex("No schedules to run.");

				try {

					init();

					if (do_measure_performance_at_start)
						start_performance_measure();
					
					while (!stop_request) {
						// wait for a user event
						// Pending async routines may release (raise) semaphores, run one now
						
						// Run all ready schedules;

						try {
							service_all_pending_aio();
                            step();

						} catch (std::error_code &ec) {
							std::cerr << "Scheduler stopped.  Reason: " << ec.message() << std::endl;
							if (ec == eng_errc::input_stream_eof)
								stop_request = true;
							else
								throw;
						}
	

						

					}
				}
				catch (std::error_code&  ec) {
					std::cerr << "Scheduler stopped after an error: " << ec.message() << std::endl;

				}
				catch (eng_ex& handled_error) {
					std::cerr << "Scheduler stopped after an error: " << handled_error.what() << std::endl;

				}
				catch (std::exception& unhandled_error) {
					std::cerr << "Scheduler stopped after an unexpected error: " << unhandled_error.what() << std::endl;

				}
				catch (...) {
					std::cerr << "Scheduler stopped after an unspecified error." << std::endl;
				}

				try {
                    // If any schedule actions are processors, run their term() routines
                    for (auto &s : schedules) {
                        auto p = dynamic_cast<processor *>(&s.action_);
                        if (p)
                            p->term(&s);
                    }
                } catch (std::exception &ex) {
				    std::cerr << "Errors during scheduler termination were ignored.\n";
				}
				stop_request = false;
#ifdef USE_ASIO
				asio_scheduler::get().stop();
#endif

                clear();
			}

		};

	} // eng
} // sel

#if defined(COMPILE_UNIT_TESTS)
#include "./unit_test.h"

SEL_UNIT_TEST(periodic_event)

		using clock = std::chrono::high_resolution_clock;

		static double now() { return static_cast<double>(clock::now().time_since_epoch().count()) / 1000000000.0; }

struct ut_traits
{

	static constexpr size_t rate = 1;
	static constexpr size_t test_duration_secs = 5;
	static constexpr size_t iters_to_run = rate * test_duration_secs;

};


struct display_frequency : sel::eng6::Processor<0, 0>
{
	double last_tick = 0;
	size_t iters_remaining = ut_traits::iters_to_run;
	sel::eng6::scheduler& scheduler_;
	display_frequency(sel::eng6::scheduler& scheduler) : last_tick(0), scheduler_(scheduler)
	{
		
	}
	
	void process() final
	{
		//const auto tick = now();
		//if (last_tick) {
		//	printf("%4.4f\n", tick - last_tick);
		//}
		if (!iters_remaining--)
			scheduler_.stop();

		//last_tick = tick;

	}
};

void run()
{
	sel::eng6::scheduler s = {};
	
	display_frequency d(s);
	sel::eng6::periodic_event p1(rate_t(ut_traits::rate, 1));

//	sel::eng6::periodic_func f1(rate_t(ut_traits::rate, 1), d);
	sel::eng6::schedule s1(&p1, d);
	s.add(s1);

	auto rate_expected = static_cast<double>(s1.expected_rate());
	printf("Test will run for %lu seconds...\n", static_cast<unsigned long>(ut_traits::test_duration_secs));

	s.do_measure_performance_at_start = true;
	
	s.run();

	const auto expected = static_cast<double>(s1.expected_rate());
	const auto actual = static_cast<double>(s1.actual_rate());

	const auto timer_error = abs(1.0 - actual / expected) + DBL_EPSILON;
	
	printf("\nRate\tClaimed: %4.4f\tActual: %4.4f\tTimer error: %4.4f%%\n", expected, actual, 100 * timer_error);
	
	SEL_UNIT_TEST_ASSERT(timer_error < 0.01)

}

SEL_UNIT_TEST_END
#endif

#endif