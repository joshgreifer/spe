#pragma once
#pragma once
#include "../eng_traits.h"
#include "../processor.h"


/*
Lattice Moving Average and Autoregressive filters
*/
namespace sel {
	namespace eng {
		namespace proc {


			/*
			Levinson-Durbin solver
			*/
			template<typename traits> 
			struct lpc :
				public  Processor<1, 3>, 
				virtual public creatable<lpc<traits>>
			{
				static constexpr size_t sz = traits::num_coefficients+1;
			public:
				
				samp_t *a_out; // output port 1 (i.e. ports[0])
				samp_t *k_out; // output port 2 (i.e. ports[1])
				samp_t *e_out; // output port 3 
				const samp_t *in;


				lpc() {}

				lpc(params& args) {}

				virtual const std::string type() const override { return "lpc"; }

			
				void process() final 
				{
					/* double e = */ levinson_durbin();
				}

				void freeze(void) final
				{

					// 
					if (inports[0]->width() < sz)
						throw "Input number of autocorrelation lags must be greater than order of lpc";

					inports[0]->freezewidth(sz);
					outports[0]->freezewidth(sz);
					outports[1]->freezewidth(sz-1);
					outports[2]->freezewidth(1);

					Connectable::freeze();

					// Once widths are set,  we can directly access underlying data, as it will not be moved any more
					in = inports[0]->as_array();
					a_out = outports[0]->as_array();
					k_out = outports[1]->as_array();
					e_out = outports[2]->as_array();

				}

			private:
				void levinson_durbin()
				{
					auto& err = *e_out;
					double Am1[sz];

					if (in[0] == 0.0) {
						for (size_t i = 0; i < sz; i++)
						{
							k_out[i] = 0.0;
							a_out[i] = 0.0;
						}
					}
					else {

						a_out[0] = 1.0;
						Am1[0] = 1.0;
						for (size_t k = 1; k < sz; k++) {
							a_out[k] = 0.0;
							Am1[k] = 0.0;
						}

						double km = 0;
						double Em = in[0];
						for (size_t m = 1; m < sz; m++)                    //m=2:N+1
						{
							err = 0.0;                    //err = 0;
							for (size_t k = 1; k <= m - 1; k++)            //for k=2:m-1
								err += Am1[k] * in[m - k];        // err = err + am1(k)*R(m-k+1);
							km = (in[m] - err) / Em;            //km=(R(m)-err)/Em1;
							k_out[m - 1] = -km;
							a_out[m] = km;                        //am(m)=km;
							for (size_t k = 1; k <= m - 1; k++)            //for k=2:m-1
								a_out[k] = Am1[k] - km * Am1[m - k];  // am(k)=am1(k)-km*am1(m-k+1);
							Em = (1 - km * km)*Em;                //Em=(1-km*km)*Em1;
							for (size_t s = 0; s < sz; s++)                //for s=1:N+1
								Am1[s] = a_out[s];                // am1(s) = am(s)


						}
						err = Em;
					}
					for (size_t m = 1; m < sz; m++)
						a_out[m] = -a_out[m];

				}
			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "lpc_ut.h"
#endif

