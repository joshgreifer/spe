//
// Created by josh on 29/10/2020.
//

#ifndef SPE_WAVFILE_H
#define SPE_WAVFILE_H
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


namespace sel {
    namespace wav {
        /*
       RIFF file format
       */
#pragma pack(push)
#pragma pack(1)

        template<size_t Sz, char Pad = '\0'>
        struct CHAR_ARRAY {
            mutable char c_[Sz];

            // For comparison, strlen(s) must == Sz,
            // which means that s must be padded with Pad
            // value. This is unlike operator=(), where
            // strlen(s) must be <= Sz.
            bool operator==(const char *s) const {
                if (!s)
                    return false;
                size_t i = 0;
                for (; i < Sz; ++i)
                    if (s[i] != c_[i])
                        return false;
                // checks strlen(s) == Sz
                return s[i] == '\0';
            }

            bool operator!=(const char *s)  const {
                return !this->operator==(s);
            }

            CHAR_ARRAY(const char s[Sz+1] = nullptr) {
                size_t i = 0;
                if (s)
                    for (; i < Sz; ++i) {
                        if (s[i] == '\0')
                            break;
                        c_[i] = s[i];
                    }

                for (; i < Sz; ++i)
                    c_[i] = Pad;
            }
        };

        // Riff Id
        typedef CHAR_ARRAY<4, ' '> RIFFId;

        struct DATAChunk {
            const RIFFId IdData = "data";
            int szData = 0;
        };

        static_assert(sizeof(DATAChunk) == 8);

        // http://soundfile.sapp.org/doc/WaveFormat/

        struct wav_file_header
                {
            static constexpr short WAVE_FORMAT_PCM = 1;
            static constexpr short WAVE_FORMAT_IEEE_FLOAT = 3;

            const RIFFId IdRIFF = "RIFF";
            int szRIFF;
            const RIFFId RiffFormat = "WAVE";

            const RIFFId IdFmt = "fmt";
            const int szFmt = 16;
            short AudioFormat = WAVE_FORMAT_PCM;
            short NumChannels = 1;
            int SampleRate = 16000;
            int ByteRate = SampleRate * NumChannels * 2 /* BitsPerSample/8 */;
            short BlockAlign = 0;
            short BitsPerSample = 16;
            DATAChunk dataChunk;

            template<class sample_t, int sample_rate, short n_channels>static wav_file_header create(size_t n_samples) {
                constexpr auto bytes_per_sample = sizeof(sample_t);
                const int data_byte_length = n_samples * bytes_per_sample * n_channels;
                wav_file_header header;

                header.szRIFF = sizeof(header) + n_samples * bytes_per_sample * n_channels;
                header.RiffFormat = "foo";
                if constexpr (std::is_floating_point_v<sample_t>)
                    header.AudioFormat = WAVE_FORMAT_IEEE_FLOAT;
                else
                    header.AudioFormat = WAVE_FORMAT_PCM;
                header.NumChannels = n_channels;
                header.SampleRate = sample_rate;
                header.ByteRate = sample_rate * n_channels * bytes_per_sample;
                header.BlockAlign = n_channels * bytes_per_sample;
                header.BitsPerSample = bytes_per_sample * 8;
                header.dataChunk.szData = data_byte_length;
                return header;
            }

            static const wav_file_header from_file(const char *file_name) {
                struct stat status;
                if (stat(file_name, &status) < 0)
                    throw std::runtime_error("Couldn't stat() wav file.");

                auto fd = open(file_name, O_RDONLY + O_BINARY);
                if (fd < 0)
                    throw std::runtime_error("Couldn't open wav file.");

                wav_file_header header;
                if (!read(fd, &header, sizeof(wav_file_header)))
                    throw sys_ex();
                close(fd);
                header.validate();
                return header;
            }

             void validate() const {


                const wav_file_header &header = *this;

                if (header.IdRIFF != "RIFF")
                    throw eng_ex("WAV File format is incorrect. (Missing 'RIFF' chunk).");

                if (header.RiffFormat != "WAVE")
                    throw eng_ex("WAV File format is incorrect. (Format is not 'WAVE').");

                if (header.IdFmt != "fmt ")
                    throw eng_ex("WAV File format is incorrect. (Missing 'fmt ' chunk).");

                if (header.AudioFormat != WAVE_FORMAT_PCM && header.AudioFormat != WAVE_FORMAT_IEEE_FLOAT)
                    throw eng_ex("The WAV file format is not supported (must be either PCM or floating-point format).");


            }

        private:

            wav_file_header() {}
        };

        static_assert(sizeof(wav_file_header) == 44);


        template<class data_type,size_t n_channels, std::enable_if_t<std::is_arithmetic<data_type>::value, int> = 0>
        std::vector<data_type>  load(const char *file_name)
        {
            std::vector<data_type>dest = {};
            struct stat status;
            if (stat(file_name, &status) < 0)
                throw std::runtime_error("Couldn't stat() wav file.");

            auto fd = open(file_name, O_RDONLY + O_BINARY);
            if (fd < 0)
                throw std::runtime_error("Couldn't open wav file.");

            char buf[sizeof(wav::wav_file_header)];
            if (!read(fd, buf, sizeof(buf)))
                throw sys_ex();

            wav_file_header header = *reinterpret_cast<wav_file_header *>(buf);
            header.validate();

            auto start_of_data = tell(fd);
            assert(start_of_data == sizeof(wav_file_header));

            auto data_len = status.st_size - start_of_data;

            if (sizeof(data_type) * 8 != header.BitsPerSample)
                throw std::runtime_error("wav file BitsPerSample mismatch");

            if constexpr (std::is_floating_point_v<data_type>) {
                if (header.AudioFormat != wav_file_header::WAVE_FORMAT_IEEE_FLOAT)
                    throw std::runtime_error("wav file format is not WAVE_FORMAT_IEEE_FLOAT");
            } else
                if (header.AudioFormat != wav_file_header::WAVE_FORMAT_PCM)
                    throw std::runtime_error("wav file format is not WAVE_FORMAT_PCM");

            if (data_len != header.dataChunk.szData)
                throw std::runtime_error("wav file data chunk size doesn't match file size");

            if (n_channels != header.NumChannels)
                throw std::runtime_error("wav file number of channels is incorrect");

            dest.resize(data_len / sizeof(data_type));
            lseek(fd, start_of_data, SEEK_SET);
            // Read entire file into memory
            if (read(fd, reinterpret_cast<char *>(dest.data()), data_len) != data_len)
                throw std::runtime_error("Couldn't read all data.");
            close(fd);
            return dest;

        }

        template<class data_type,size_t n_channels, size_t sample_rate, std::enable_if_t<std::is_arithmetic<data_type>::value, int> = 0>
          void save(std::vector<data_type> data, const char *file_name)
          {
              auto n_values = data.size();
              auto header = wav_file_header::create<data_type, sample_rate, n_channels>();

              FILE* out = fopen(file_name, "wb");
              fwrite(header, 1, sizeof(header), out);
              auto el_size = sizeof(data[0]);

              auto n_written = fwrite(&data[0], el_size, n_values, out);
              if (n_written != n_values)
                  perror(nullptr);
              fclose(out);
          }
#pragma pack(pop)


    }
}

#if defined(COMPILE_UNIT_TESTS)
#include "unit_test.h"
SEL_UNIT_TEST(wavfile)

        void run()
        {
        const std::string test_file = "test_audio_16k_i16.wav";
        auto v =  sel::wav::load<short, 1>(test_file.c_str());

//            SEL_UNIT_TEST_ASSERT(data1 == data2);


        }
SEL_UNIT_TEST_END
#endif
#endif