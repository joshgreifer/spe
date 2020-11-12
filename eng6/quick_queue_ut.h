#pragma once

// mux_demux and demux unit tests
#include "quick_queue.h"
#include <algorithm>
#include <array>
#include "unit_test.h"
SEL_UNIT_TEST(quick_queue)


struct ut_traits
{
	using item_type = double;

	static constexpr size_t fixed_queue_size = 19;
	static constexpr size_t dynamic_queue_size = 17;
	static constexpr size_t block_size = 13;
	static constexpr size_t iters = 1000;

};

using qq_constexpr_sz = sel::quick_queue<ut_traits::item_type, ut_traits::fixed_queue_size>;
using qq_dynamic_sz = sel::quick_queue<ut_traits::item_type>;

//		using mux_demux = sel::eng6::proc::mux_demux <ut_traits::input_size, ut_traits::output_size>;





void run() 
{

	try {

		// templated size (ut_traits::fixed_queue_size)
		qq_constexpr_sz q1;

		// Okay, constructor size == templated size
		qq_constexpr_sz q2(ut_traits::fixed_queue_size);

		// Ok, constructor size <= templated size
		qq_constexpr_sz q3(ut_traits::dynamic_queue_size);

		// Error, constructor size > templated size
		// qq_constexpr_sz q3(ut_traits::fixed_queue_size+1);

		qq_dynamic_sz q4(ut_traits::dynamic_queue_size);

		size_t c = 0;

		std::array<ut_traits::item_type, ut_traits::block_size> block_data, block_data_check;

		for (size_t i = 0; i < ut_traits::iters; ++i)
		{
			std::generate(block_data.begin(), block_data.end(), [&] ()  { return static_cast<ut_traits::item_type>(c++); });
			block_data_check = block_data;
			q1.atomicwrite(block_data);
			q1.atomicread_into(block_data);
			SEL_UNIT_TEST_ASSERT(block_data == block_data_check);

			// try async write
			auto *p = q2.acquirewrite();
		
			for (auto q = block_data.begin(); q != block_data.end(); )
				*p++ = *q++;
			q2.endwrite(block_data.size());


			q2.atomicread_into(block_data);
			SEL_UNIT_TEST_ASSERT(block_data == block_data_check);

			q3.atomicwrite(block_data);
			q3.atomicread_into(block_data);
			SEL_UNIT_TEST_ASSERT(block_data == block_data_check);

			q4.atomicwrite(block_data);
			q4.atomicread_into(block_data);					
			SEL_UNIT_TEST_ASSERT(block_data == block_data_check);


			// fill a buffer right up
			while (q1.put_avail())
				q1.put(0.0);
			SEL_UNIT_TEST_ASSERT(q1.isfull());
			SEL_UNIT_TEST_ASSERT(!q1.isempty());
			SEL_UNIT_TEST_ASSERT( q1.acquireread() == q1.acquirewrite());

			// now empty it 
			while (q1.get_avail())
				q1.get();
			SEL_UNIT_TEST_ASSERT(!q1.isfull());
			SEL_UNIT_TEST_ASSERT(q1.isempty());
			SEL_UNIT_TEST_ASSERT( q1.acquireread() == q1.acquirewrite());

		}



	} catch (std::runtime_error &ex) {
		std::cout << ex.what() << std::endl;
	}
}

SEL_UNIT_TEST_END

