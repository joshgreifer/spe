#pragma once
#include "../new_processor.h"

#include "../../eng6/numpy.h"

namespace sel
{
    namespace eng7
    {
        namespace proc
        {

            template<class data_t, size_t ArrayWidth>class numpy_file_reader : public  data_source<ArrayWidth, samp_t>
            {
                std::vector<data_t> v;

                const std::string file_name;
                size_t rows_in_file  = 0;
                size_t rows_read = 0;
                size_t offset  = 0;

            public:
                void process() final
                {
                    if (rows_read++ >= rows_in_file) {
                        throw std::error_code(eng_errc::input_stream_eof);
                    }
                    for (size_t i = 0 ; i < ArrayWidth; ++i)
                        this->out()[i] = static_cast<samp_t>(v[offset++]);

                }

                numpy_file_reader(const std::string& file_name) : file_name(file_name)
                {
                    // Load the numpy data
                   v = sel::numpy::load<data_t>(file_name.c_str());
                    rows_in_file = v.size() / ArrayWidth;
                    if (rows_in_file *  ArrayWidth != v.size())
                        throw eng_ex("Numpy file does not contain a whole number of rows. Shape is incorrect");

                    // raise semaphore count (+1 so that  EOF will get signalled)
                    this->raise(rows_in_file+1);
                }

            };
        }
    }
}