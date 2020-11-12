#pragma once
#include "../processor.h"


namespace sel {
	namespace eng6 {
		namespace proc {

			template<typename traits>struct proc_impl
			{
			
				// Internal state 

				proc_impl()
				{


				}

			};

			template<typename traits=void, typename impl_t=void> 
			struct $safeitemname$ : public  Processor1A1B<1, 1>, virtual public creatable<$safeitemname$<traits, impl_t>>
			{

				impl_t impl_{};

			public:

				$safeitemname$() {}
				$safeitemname$(params& args) {}

				virtual const std::string type() const override { return impl_t::name(); }

			
				void process() final 
				{
				}
			};

		} // proc
	} // eng
} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "template_proc_ut.h"
#endif


