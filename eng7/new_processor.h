#pragma once
#include <tuple>
#include "../eng/event.h"
#include "../eng/func.h"


namespace magic {
	// From https://gist.github.com/sighingnow/505d3d5c82237741b4a18147b2f84811

	template <template <typename...> class T, typename U>
	struct is_specialization_of : std::false_type {};

	template <template <typename...> class T, typename... Us>
	struct is_specialization_of<T, T<Us...>> : std::true_type {};

	// example: is_tuple
	template <typename T>
	struct is_tuple : is_specialization_of<std::tuple, typename std::decay<T>::type> {};
}

namespace sel {
	namespace eng7 {
		class schedule;
		struct processor : public functor
		{

			void operator()() final { process(); }
			virtual void process() = 0;
			virtual void init(schedule* context) {};
			virtual void term(schedule* context) {};

			virtual ~processor() {}
		};
		
		class None {};

		template<class in_ports_t, class outports_t> struct processor7 : processor
		{
			static constexpr bool has_inputs = magic::is_tuple<in_ports_t>::value;
			static constexpr bool has_outputs = magic::is_tuple<outports_t>::value;

			in_ports_t inports;
			outports_t outports;

			template<size_t pin = 0, class = typename std::enable_if<magic::is_tuple<in_ports_t>::value>::type> auto& in() {
				static_assert(has_inputs, "Processor has no inputs.");
				return std::get<pin>(this->inports);
			}
			template<size_t pin = 0, class = typename std::enable_if<magic::is_tuple<in_ports_t>::value>::type> const auto in_v() const {
				static_assert(has_inputs, "Processor has no inputs.");
				auto p = std::get<pin>(this->inports);
				if (!p)
					throw std::runtime_error("Attempt to access unconnected input.");
				return *p; 
			}
			template<size_t pin = 0, class = typename std::enable_if<magic::is_tuple<outports_t>::value>::type> auto& out() {
				static_assert(has_outputs, "Processor has no outputs.");

				return std::get<pin>(this->outports); 
			}
			template<size_t pin = 0, class = typename std::enable_if<magic::is_tuple<outports_t>::value>::type> auto out() const {
				static_assert(has_outputs, "Processor has no outputs.");

				return std::get<pin>(this->outports); 
			}

			template<class TO_PROC, size_t from_pin = 0, size_t to_pin = 0> void connect_to(TO_PROC& to) {
				static_assert(TO_PROC::has_inputs, "Can't connect: 'to' Processor has no inputs.");
				static_assert(has_outputs, "Can't connect: 'from' Processor has no outputs.");

				to.in<to_pin>() = &this->out<from_pin>();
			}

		};

		template<size_t N_IN, size_t N_OUT>struct stdproc: processor7<
			std::tuple< const std::array<samp_t, N_IN>*>,
			std::tuple< std::array<samp_t, N_OUT> > >
		{
			static constexpr auto input_width = N_IN;
			static constexpr auto output_width = N_OUT;
		};
		

		template<size_t N_OUT>struct stdsource:
			processor7<
			None,
			std::tuple< std::array<samp_t, N_OUT> > >
		{
			static constexpr auto input_width = 0;
			static constexpr auto output_width = N_OUT;

		};


		template<size_t N_IN>struct stdsink :
			processor7<
			std::tuple< std::array<samp_t, N_IN> *>,
			None
			>
		{
			static constexpr auto input_width = N_IN;
			static constexpr auto output_width = 0;

		};

		template<size_t OUTW>class data_source : public stdsource<OUTW>, public semaphore {
		public:
			//				virtual const std::string type() const override { return "data_source"; }
			data_source(rate_t expected_rate = rate_t()) : semaphore(0, expected_rate) {}
			void enable(bool enabled = true) const { semaphore::enabled = enabled; }
			void invoke(size_t repeat_count = 1) { schedule(this, *this).invoke(repeat_count); }
		};
	}
}
