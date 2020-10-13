#pragma once
#include <vector>
#include <string>
#include "unit_test.h"

namespace sel {
	namespace numpy
	{
		/**
		 * Diff
		 */
		std::vector<double> diff(std::vector<double>& vec)
		{
			std::vector<double> diffed;
			for (size_t i = 0; i < vec.size()-1; ++i)
				diffed.push_back(vec[i + 1] - vec[i]);
			
			return diffed;
		}
		
		/**
		 * Linspace
		 */
	    // https://stackoverflow.com/questions/27028226/python-linspace-in-c
		template<typename T>
		std::vector<double> linspace(T start_in, T end_in, size_t num_in)
		{

			std::vector<double> linspaced;

			double start = static_cast<double>(start_in);
			double end = static_cast<double>(end_in);
			double num = static_cast<double>(num_in);

			if (num == 0) { return linspaced; }
			if (num == 1)
			{
				linspaced.push_back(start);
				return linspaced;
			}

			double delta = (end - start) / (num - 1);

		
			for (int i = 0; i < num - 1; ++i)
			{
				linspaced.push_back(start + delta * i);
			}
			linspaced.push_back(end); // I want to ensure that start and end
									  // are exactly the same as the input
			return linspaced;
		}

		template<class N, bool = std::is_arithmetic<N>::value>
		class np_type
		{
			static N t;
			bool isSigned = std::is_signed<N>::value;
			char sizeChar = '0' + sizeof(t);
			bool isIntegral = std::is_integral<N>::value;
			char type = isIntegral ?
				(sizeof(t) == 1 ? (isSigned ? 'b' : 'B') : 'i') : 'f';
		public:
			const char value[4] = { '<', type, sizeChar, '\0' };
		};
		/**
		 * Save
		 */
		template<class N, std::enable_if_t<std::is_arithmetic<N>::value, int> = 0>
		void save(const N *data, const char *file_name, std::vector<int>shape)
		{
			// get numpy name
			np_type<N> t;
			const char *type = t.value;
			const char* magic = "\x93NUMPY";
			const size_t header_len = 128;
	
			
			std::string shape_str = "(";
			size_t n_values = 1;
			for (auto dim : shape) {
				n_values *= dim;
				shape_str += std::to_string(dim);
				shape_str += ", ";
			}
			shape_str += ")";
			
			char header[header_len];
			//for (size_t i = 0; i < header_len; ++i)
			//	header[i] = ' ';
			
			snprintf(header, header_len, "%s%c%c%c%c{'descr': '%s', 'fortran_order': False, 'shape': %s, }",
				magic, 1, 0, static_cast<char>(header_len - strlen(magic) - 4), 0, type, shape_str.c_str()

				);

			// Get rid of trailing eos
			size_t i = header_len - 1;
			for (; header[i]; --i)
				header[i] = ' ';
			header[i] = ' ';
			
			FILE* out = fopen(file_name, "wb");
			fwrite(header, 1, sizeof(header), out);
			auto el_size = sizeof(data[0]);
			
			auto n_written = fwrite((void *)(&data[0]), el_size, n_values, out);
			if (n_written != n_values)
				perror(nullptr);
			fclose(out);
			
		}


		
	}
}
#if defined(COMPILE_UNIT_TESTS)
SEL_UNIT_TEST(numpy)
void run() {
	std::vector<int>shape = { 2, 3, 4 };
	double data[2 * 3 * 4] = {};
	int d = 0;
	for (auto& c : data)
		c = d++;
	
	sel::numpy::save<double>(data, "test.npy", shape);
	SEL_UNIT_TEST_ASSERT(true);
}
SEL_UNIT_TEST_END
#endif