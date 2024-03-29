#pragma once
#include "../new_processor.h"
#include "../../eng6/numpy.h"
/*
 * Saves output as numpy file 2d array,  shape [ArrayWidth,  n]
 */
namespace sel
{
	namespace eng7
	{
		namespace proc
		{

			template<class data_t, size_t ArrayWidth, size_t max_rows = 1000000>class numpy_file_writer : public stdsink<ArrayWidth>
			{
				std::vector<data_t> v;

				const std::string file_name;
				size_t rows_acquired  = 0;
				
			public:
				void process() final
				{

                    if (++rows_acquired <= max_rows)
                        for (size_t i = 0; i < ArrayWidth; ++i)
						    v.push_back(static_cast<data_t>(this->in_v()[i]));

				}

				numpy_file_writer(const std::string& file_name) : file_name(file_name)
				{

				}

				void term(sel::eng6::schedule* context) final
				{
					sel::numpy::save(v.data(), file_name.c_str(), { ArrayWidth, static_cast<int>(v.size() / ArrayWidth) });
				}
			};
		}
	}
}

