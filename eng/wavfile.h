//
// Created by josh on 29/10/2020.
//

#ifndef SPE_WAVFILE_H
#define SPE_WAVFILE_H


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
                if (std::is_floating_point_v<sample_t>)
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

            static const wav_file_header & from_bytes(const char *bytes) {


                const wav_file_header &header = *reinterpret_cast<const wav_file_header *>(bytes);

                if (header.IdRIFF != "RIFF")
                    throw eng_ex("WAV File format is incorrect. (Missing 'RIFF' chunk).");

                if (header.RiffFormat != "WAVE")
                    throw eng_ex("WAV File format is incorrect. (Format is not 'WAVE').");

                if (header.IdFmt != "fmt ")
                    throw eng_ex("WAV File format is incorrect. (Missing 'fmt ' chunk).");



                if (header.AudioFormat != WAVE_FORMAT_PCM && header.AudioFormat != WAVE_FORMAT_IEEE_FLOAT)
                    throw eng_ex("The WAV file is not PCM or floating-point format.");

                return header;
            }

        private:

            wav_file_header() {}
        };

        static_assert(sizeof(wav_file_header) == 44);

#pragma pack(pop)


    }
}
#endif