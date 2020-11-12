#pragma once
#include "../processor.h"
#include "../numpy.h"
/*
 * Saves output as numpy file 2d array,  shape [ArrayWidth,  n]
 */
namespace sel
{
	namespace eng6 {
		namespace proc
		{

			template<class data_t, size_t ArrayWidth, size_t max_rows = 1000000>class numpy_file_writer : public sel::eng6::Processor1A0<ArrayWidth>
			{
				std::vector<data_t> v;

				const std::string file_name;
				size_t rows_acquired  = 0;
				
			public:
				void process() final
				{

                    if (++rows_acquired <= max_rows)
                        for (size_t i = 0; i < ArrayWidth; ++i)
						    v.push_back(static_cast<data_t>(this->in[i]));

				}

				numpy_file_writer(const std::string& file_name) : file_name(file_name)
				{

				}

				void term(schedule* context) final
				{
					numpy::save(v.data(), file_name.c_str(), { ArrayWidth, static_cast<int>(v.size() / ArrayWidth) });
				}
			};
		}
	}
}

