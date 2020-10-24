#pragma once
#include <tuple>
#include "../eng/scheduler.h"
#include "../eng/event.h"
#include "../eng/func.h"

#include <eigen3/Eigen/Core>
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
constexpr size_t dynamic_size_v=std::numeric_limits<std::size_t>::max();

namespace sel {
	namespace eng7 {
		struct processor : public functor
		{

			void operator()() final { process(); }
			virtual void process() = 0;
			virtual void init(eng::schedule* context) {};
			virtual void term(eng::schedule* context) {};

			virtual ~processor() {}
		};
		
		class None {};

		template<class in_ports_t, class out_ports_t> struct processor7 : processor
		{
			static constexpr bool has_inputs = magic::is_tuple<in_ports_t>::value;
			static constexpr bool has_outputs = magic::is_tuple<out_ports_t>::value;

			in_ports_t inports;
			out_ports_t outports;

//            void set_inputs_to_nullptr() {
//                auto &f = [](const void *p) -> int { p = 0; return 0; };
//                std::apply([f](auto&&... args) {(f(args), ...);}, inports);
//            }
//


			template<size_t pin = 0, typename in_t=in_ports_t, class = typename std::enable_if<magic::is_tuple<in_t>::value>::type> auto& in() {
				static_assert(has_inputs, "Processor has no inputs.");
				return std::get<pin>(this->inports);
			}
			template<size_t pin = 0,  typename in_t=in_ports_t, class = typename std::enable_if<magic::is_tuple<in_t>::value>::type> const auto in_v() const {
				static_assert(has_inputs, "Processor has no inputs.");
				auto p = std::get<pin>(this->inports);
				if (!p)
					throw std::runtime_error("Attempt to access unconnected input.");
				return *p; 
			}
			template<size_t pin = 0, typename out_t=out_ports_t, class = typename std::enable_if<magic::is_tuple<out_t>::value>::type> auto& out() {
				static_assert(has_outputs, "Processor has no outputs.");

				return std::get<pin>(this->outports); 
			}
            template<size_t pin = 0, typename out_t=out_ports_t, class = typename std::enable_if<magic::is_tuple<out_t>::value>::type> auto out() const {
				static_assert(has_outputs, "Processor has no outputs.");

				return std::get<pin>(this->outports); 
			}

			template<class TO_PROC, size_t from_pin = 0, size_t to_pin = 0> void connect_to(TO_PROC& to) {
				static_assert(TO_PROC::has_inputs, "Can't connect: 'to' Processor has no inputs.");
				static_assert(has_outputs, "Can't connect: 'from' Processor has no outputs.");

				to.template in<to_pin>() = &this->out<from_pin>();
			}

		};

		template<size_t N_IN, size_t N_OUT, class input_t=samp_t, class output_t=samp_t>struct stdproc: processor7<
			std::tuple< const std::array<input_t, N_IN>*>,
			std::tuple< std::array<output_t, N_OUT> > >
		{
			static constexpr auto input_width = N_IN;

			auto output_width() const
			{
			    return N_OUT;
			}
		};


		template<size_t N_OUT, class output_t=samp_t>struct stdsource:
			processor7<
			None,
			std::tuple< std::array<output_t, N_OUT> > >
		{
			static constexpr auto input_width = 0;
            auto output_width() const
            {
                return N_OUT;
            }

		};

        template<class output_t> struct stdsource<dynamic_size_v, output_t>:
                processor7<
                        None,
                        std::tuple< std::vector<samp_t> > >
        {
            static constexpr auto input_width = 0;
            auto output_width() const
            {
                return out<0>().size();
            }

        };

		template<size_t N_IN, class input_t>struct stdsink :
			processor7<
			std::tuple< std::array<input_t, N_IN> *>,
			None
			>
		{
			static constexpr auto input_width = N_IN;
            auto output_width() const
            {
                return 0;
            }


		};

		template<size_t OUTW, class output_t>class data_source : public stdsource<OUTW, output_t>, public eng::semaphore {
		public:
			//				virtual const std::string type() const override { return "data_source"; }
			data_source(rate_t expected_rate = rate_t()) : eng::semaphore(0, expected_rate) {}
			void enable(bool enabled = true) const { eng::semaphore::enabled = enabled; }
			void invoke(size_t repeat_count = 1) { eng::schedule(this, *this).invoke(repeat_count); }
		};
	}
}
