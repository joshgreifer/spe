#pragma once
#include "../processor.h"
/*
 * Saves output as numpy file 2d array,  shape [ArrayWidth,  n]
 */
namespace sel
{
	namespace eng
	{
		namespace proc
		{

			template<size_t ArrayWidth>class numpy_file_writer : public sel::eng::Processor1A0<ArrayWidth>
			{
				std::vector<samp_t> v;

				const std::string file_name;
				
			public:
				void process() final
				{

					for (size_t i = 0; i < ArrayWidth; ++i)
						v.push_back(this->in[i]);

				}

				numpy_file_writer(const std::string& file_name) : file_name(file_name)
				{

				}

				void term(sel::eng::schedule* context) final
				{
					sel::numpy::save(v.data(), file_name.c_str(), { ArrayWidth, static_cast<int>(v.size() / ArrayWidth) });
				}
			};
		}
	}
}