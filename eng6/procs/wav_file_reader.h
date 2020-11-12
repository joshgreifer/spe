//
// Created by josh on 30/10/2020.
//

#ifndef SPE_WAV_FILE_READER_H
#define SPE_WAV_FILE_READER_H
#pragma once
#include "../processor.h"
#include "data_source.h"
#include "../wavfile.h"

namespace sel
{
    namespace eng6
    {
        namespace proc
        {

            template<class data_t, size_t nChannels, size_t ArrayWidth>class wav_file_reader : public Processor<0, nChannels>, public semaphore
            {
                static constexpr auto  RowSize = nChannels * ArrayWidth;
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
                        for (size_t chan = 0; chan < nChannels; ++chan)
                            this->outports[chan]->as_array()[i] = static_cast<samp_t>(v[offset++]);
                }

                wav_file_reader(const std::string& file_name) : file_name(file_name)
                {
                    // Read the header
                    auto header =  sel::wav::wav_file_header::from_file(file_name.c_str());
                    this->set_rate(header.SampleRate, 1);

                    v = sel::wav::load<data_t, nChannels>(file_name.c_str());
                    rows_in_file = v.size() / RowSize;
                    if (rows_in_file *  RowSize != v.size())
                        // pad with zeros
                        v.resize(v.size() + (RowSize - v.size() % RowSize) % RowSize, 0);


                    // raise semaphore count (+1 so that  EOF will get signalled)
                    raise(rows_in_file+1);
                }

            };
        }
    }
}

#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"
SEL_UNIT_TEST(wav_file_reader)

        void run()
        {
        const std::string test_file = "test_audio_16k_i16.wav";

        sel::eng6::proc::wav_file_reader<short, 1, 1024> mono16bit_reader(test_file.c_str());

        auto v =  sel::wav::load<short, 1>(test_file.c_str());

        SEL_UNIT_TEST_ASSERT(mono16bit_reader.rate().expected() == 16'000);


        }
SEL_UNIT_TEST_END
#endif

#endif //SPE_WAV_FILE_READER_H
