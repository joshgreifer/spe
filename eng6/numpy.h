#pragma once
#ifndef SPE_NUMPY_H
#define SPE_NUMPY_H
#include <fcntl.h>
#ifdef _WIN32
#ifndef _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <io.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #define tell(fd) lseek(fd, 0, SEEK_CUR)
    #define O_BINARY 0 /* Not available in Linux */
#endif
#include <vector>
#include <string>
#include "msg_and_error.h"

template<typename T>
struct is_complex_t : public std::false_type {};
template<typename T>
struct is_complex_t<std::complex<T>> : public std::true_type {};

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
		template<typename T, typename internal_T=double>
		std::vector<internal_T> linspace(T start_in, T end_in, size_t num_in)
		{

			std::vector<internal_T> linspaced;

			double start = static_cast<internal_T>(start_in);
			double end = static_cast<internal_T>(end_in);
			double num = static_cast<internal_T>(num_in);

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
            bool isComplex = is_complex_t<N>::value;
            bool isSigned = std::is_signed<N>::value;
            char sizeChar = '0' + sizeof(t);
            bool isIntegral = std::is_integral<N>::value;
            char type = isIntegral ?
                        (sizeof(t) == 1 ? (isSigned ? 'b' : 'B') : 'i') : isComplex ? 'c' : 'f';
            charbuf<4> buf;
        public:
            const char *value = buf.sprintf("<%c%d", type, static_cast<unsigned int>(sizeof(t)));
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

        template<class N, std::enable_if_t<std::is_arithmetic<N>::value, int> = 0>
        std::vector<N>  load(const char *file_name)
        {
            std::vector<N>dest = {};
            const char* magic = "\x93NUMPY";
            struct stat status;
            if (stat(file_name, &status) < 0)
                throw std::runtime_error("Couldn't stat() numpy file.");
            auto fd  = open(file_name, O_RDONLY + O_BINARY);

            if (fd < 0)
                throw std::runtime_error("Couldn't open numpy file.");

            constexpr size_t pre_header_length = 10;
            char buf[pre_header_length];

            if (pre_header_length != read(fd, buf, pre_header_length ))
                throw std::runtime_error("Not a numpy file (couldn't read header");
            if (strncmp(buf, magic, strlen(magic)))
                throw std::runtime_error("Not a numpy file (magic value missing)");

            auto header_len = static_cast<size_t>(buf[8]) + 256 * static_cast<size_t>(buf[9]);
            auto start_of_data = pre_header_length + header_len;
            auto data_len = status.st_size - start_of_data;

            dest.resize(data_len / sizeof(N));
            lseek(fd, start_of_data, SEEK_SET);
            // Read entire file into memory
            if (read(fd, reinterpret_cast<char *>(dest.data()), data_len) != data_len)
                throw std::runtime_error("Couldn't read all data.");
            close(fd);
            return dest;

        }
	}
}
#if defined(COMPILE_UNIT_TESTS)
#include "procs/numpy_ut.h"
#endif
#endif