#pragma once
#include "compound_processor.h"
#include "rand.h"

namespace sel {
	namespace eng {
		namespace proc {
			namespace sample {

				/*
				Processor class naming convention:

				Processor01x - Processor with no input ports and one output port of variable width
				Processor01A<N> - Processor with no input ports and one output port with fixed width N
				Processor1x1x -  Processor with  one input port and one output port, with the input port variable width and the output port width set to be the same as the input width
				Processor1A1B<N,M> -  Processor with  one input port and one output port, with the port widths fixed at N, M respectively

				... more complex cases:

				Processor2Ax11<N> - Processor with two input ports and one output port.  The first input port width is N, the second is variable.  The output port width is 1 (scalar).
				ProcessorX121 - Processor with variable number of input ports each of width 1 (scalar values) and two output ports of width 1.
				Processor421x90 - Processor with four input ports, of widths two, one, variable and nine, and no output ports
				*/



				/*
				Sample Sink Processor: ProcessorXx0 (Variable number of inputs each of variable width, no outputs)
				*/
				struct Logger : virtual ProcessorXx0, virtual public creatable<Logger>
				{
					std::ostream& ostream = std::cout;

					virtual const std::string type() const override { return "Logger"; }
					explicit Logger(std::ostream& ostr = std::cout)  :  ostream(ostr) {
						set_port_alias("Logger.in", PORTID_0);
					}
					explicit Logger(params& params) : Logger() {}

					void process() override
					{
						for (size_t i = 0; i < inports.size(); ++i) {
							std::cout << "Port " << i + 1 << ": [ ";
							for (auto v : *inports[i])
								std::cout << v << " ";
							std::cout << "]" << std::endl;

						}
					}

				};

				/*
				Sample variable width in-out Processor: Processor1x1x (One input of variable width, one output with the same width as input)
				*/
				struct Doubler : Processor1x1x
				{

					void process() override
					{

						for (size_t i = 0; i < width; ++i)
							out[i] = 2.0 * in[i];
					}
				};

				/*
				Sample aggregator Processor: Processor1x11 (One input of variable width, one output with scalar value)
				*/
				struct Summer : Aggregator
				{

					void process() override
					{
						outv = 0;
						for (size_t i = 0; i < width; ++i)
							outv += in[i];
						outv = 0;
						for (auto v : *piport) {
							outv += v;
						}
					}
				};

				struct MyCompoundProc : public compound_processor
				{
					sel::eng::proc::rand<7> rng;
					Logger logger;
					double num[3] = { 1, 4, 9 };
					Const c = Const(&num[0], &num[3]);
					Summer s;
					virtual const std::string type() const override { return "MyCompoundProc"; }
					MyCompoundProc() {
						// No inputs
						//AddInputProc(&c1);
						// Output of this proc is the output of the random number generator
//						add_output_proc(rng);
						connect_procs(rng, logger);
						connect_const(c, s);
						connect_const(c, logger);
						connect_procs(s, logger);
					}

				};
			} // sample
		} // proc
	} // eng
} // sel
