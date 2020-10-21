#pragma once
#include <vector>
#include <array>
#include <functional>
#include "func.h"

#include "connectable.h"
#include "event.h"

#include "dag.h"

//using namespace std;
namespace sel {



	namespace eng {
		class schedule;


		struct processor : public functor
		{

			void operator()() final { process(); }
			virtual void process() = 0;
			virtual void init(schedule *context) {};
			virtual void term(schedule *context) {};

			virtual ~processor() {}
		};


		struct ConnectableProcessor : public processor, public Connectable<samp_t>, public traceable<ConnectableProcessor>
		{

			virtual ConnectableProcessor& input_proc()  { return *this; }
			virtual ConnectableProcessor& output_proc() { return *this; }

			virtual std::ostream& trace(std::ostream& os) const override
			{
				return Connectable::trace(os);
			}

			virtual ~ConnectableProcessor() {}
		};

		class Const : public Connectable<samp_t>,  public creatable<Const>
		{
			port value;
		public:
			virtual const std::string type() const override { return "const"; }
			double at(size_t idx) const { return value.as_array()[idx];  }
			double& at(size_t idx) { return value.as_array()[idx];  }
			operator double() const { return value.as_array()[0]; }
			
			Const& operator= (double other)  { value.as_array()[0] = other; return *this; }

			void freeze(void) override
			{
				if (!is_input_connected(PORTID_ALL))
					throw sp_ex_input_port_notconnected();
			}


			Const() = default;

			Const(double v) {
				value.as_array()[0] = v;
				value.freeze();
				outports.add(&value);
				inports.freeze();
				outports.freeze();

			}

			Const(params& params) : Const(params.get<std::vector<double>>("value")) { }

			template<class IT>Const(const IT first, const IT last) : value(first, last) {
				outports.add(&value);
				value.freeze();
				inports.freeze();
				outports.freeze();
			}
			// Iterable class
			template<class T>Const(const T& values) : value(values.begin(), values.end()) {
				outports.add(&value);
				value.freeze();
				inports.freeze();
				outports.freeze();
			}

		};

		template<size_t N_IN, size_t N_OUT>struct Processor : public ConnectableProcessor
		{
			std::array<port, N_OUT> o_;

			void _configure_ports()
			{
				inports.add(nullptr, N_IN);
				for (auto& port : o_)
					outports.add(&port);
				this->outports.freeze();
				this->inports.freeze();

			}


			Processor()
			{
				_configure_ports();

			}
		};
		//template<size_t N_OUT>struct Processor<1, N_OUT> : public ConnectableProcessor
		//{
		//	const double *in = nullptr;

		//	Processor()
		//	{
		//		inports.add(nullptr, 1);
		//	}

		//	void freeze(void) override
		//	{

		//		Connectable::freeze();

		//		// Once widths are set,  we can directly access underlying data, as it will not be moved any more
		//		in = inports[0]->as_array();

		//	}

		//};
		template<size_t N_IN>struct Processor<N_IN, 0> : public ConnectableProcessor
		{
			Processor()
			{
				inports.add(nullptr, N_IN);
				this->outports.freeze();
			}
		};

		template<>struct Processor<ConnectableProcessor::PORTID_NEW, 0> : public ConnectableProcessor
		{
			virtual size_t default_inport() const { return ConnectableProcessor::PORTID_NEW; }

			Processor()
			{
				this->outports.freeze();
			}
		};


		template<>struct Processor<ConnectableProcessor::PORTID_NEW, ConnectableProcessor::PORTID_NEW> : public ConnectableProcessor
		{
			virtual size_t default_inport() const { return ConnectableProcessor::PORTID_NEW; }
			//	virtual size_t default_outport() const { return ConnectableProcessor::PORTID_NEW; }

			Processor() {}
		};

		typedef struct Processor<ConnectableProcessor::PORTID_NEW, ConnectableProcessor::PORTID_NEW> ProcessorXxYy;

		typedef struct Processor<ConnectableProcessor::PORTID_NEW, 0> ProcessorXx0;


		struct Processor1x1x : public Processor<1, 1>
		{

			// 
			size_t width;
			const double *in;
			double *out;

			Processor1x1x() {

			}
			void freeze(void) override
			{
				width = inports[0]->width();
				outports[0]->setwidth(width); // set outport port to same width as input port

				Connectable::freeze();

				// Once widths are set,  we can directly access underlying data, as it will not be moved any more
				in = inports[0]->as_array();
				out = outports[0]->as_array();

			}
		};

		template<size_t OUTW>struct Processor1x1A : public Processor<1, 1>
		{

			static_assert(OUTW, "Processor1x1A: OUTW == 0");
			// 
			size_t width;
			const double *in;
			port& oport;
			double *out;
			double &outv;
			port *piport;

			Processor1x1A() :
				oport(*outports[0]),
				out(oport.as_array()),
				outv(out[0])
			{
				oport.freezewidth(OUTW); // set outport port width to templated value

			}
			void freeze(void) override
			{

				Connectable::freeze();
				// Once widths are set,  we can directly access underlying data, as it will not be moved any more
				piport = inports[0];
				width = piport->width();
				in = piport->as_array();

			}
		};

		typedef Processor1x1A<1> Aggregator;

		template<size_t INW0, size_t OUTW0>struct Processor1A1B : public Processor<1, 1>
		{
			const double *in;
			double *out;
			port *piport;
			port& oport;

			Processor1A1B() : oport(*outports[0]) {

			}

			void freeze(void) override
			{

				// 
				piport = inports[0];
				if (piport->width() != INW0)
					throw sp_ex_pin_arity();
				piport->freezewidth(INW0);

				oport.freezewidth(OUTW0);
				Connectable::freeze();


				// Once widths are set,  we can directly access underlying data, as it will not be moved any more
				in = piport->as_array();
				out = oport.as_array();

			}
		};

		typedef Processor1A1B<1, 1> ScalarProc;

		template<size_t OUTW0>struct Processor01A : public Processor<0, 1>
		{
			// 
			static constexpr size_t width = OUTW0;
			double *out;
			port& oport;

			Processor01A() : oport(*outports[0])
			{
				// Can't change width of output
				oport.freezewidth(OUTW0);
				out = oport.as_array();
				// Freeze output ports to one and only one port
				outports.freeze();

			}
			void freeze(void) override
			{

				Connectable::freeze();

				// Once widths are set,  we can directly access underlying data, as it will not be moved any more


			}
		};
		
		template<size_t OUTW>using Source = Processor01A<OUTW>;

		template<size_t INW0>struct Processor1A0 : public Processor<1, 0>
		{
			// 
			static constexpr size_t width = INW0;
			const double *in;
			port *piport;

			Processor1A0() 
			{
			}
			void freeze(void) override
			{
				piport = inports[0];
				piport->freezewidth(INW0);

				in = piport->as_array();

				Connectable::freeze();

				// Once widths are set,  we can directly access underlying data, as it will not be moved any more


			}
		};

		template<size_t INW>using Sink = Processor1A0<INW>;


		template<size_t InWidth, size_t OutWidth>struct rate_changer : public semaphore
		{
			
		};
	} // eng
} // sel