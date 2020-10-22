#pragma once
#include <cinttypes>
#include "data_source.h"
#include "../file_input_stream.h"

namespace {

	typedef int16_t SAMP16;

#if SUPPORTS_STEREO
#pragma pack(push)
#pragma pack(2)
	template<typename SIGNAL_T>class  StereoSamp
	{
		union _s {
			struct _chan {
				SAMP16 L;
				SAMP16 R;
			} chan;
			DWORD i32data;
		} s;
	public:
		inline SAMP16& L() { return s.chan.L; }
		inline SAMP16& R() { return s.chan.R; }

		inline StereoSamp(SAMP16 left = 0, SAMP16 right = 0)
		{
			s.chan.L = left;
			s.chan.R = right;

		}

		inline StereoSamp(SIGNAL_T left, SIGNAL_T right)
		{
			s.chan.L = (SAMP16)(left * 32767 + 0.5);
			s.chan.R = (SAMP16)(right * 32767 + 0.5);
		}
	};
#pragma pack(pop)
#endif
	/*
	RIFF file format
	*/
#pragma pack(push)
#pragma pack(1)

	template <size_t Sz, char Pad = '\0'> struct CHAR_ARRAY {
		char c_[Sz];

		bool operator==(const char* s)
		{
			if (!s)
				return false;
			size_t i = 0;
			for (; i < Sz; ++i)
				if (s[i] != c_[i])
					return false;
			// checks strlen(s) == Sz
			return s[i] == '\0';
		}

		bool operator!=(const char* s)
		{
			return !this->operator==(s);
		}

		CHAR_ARRAY(const char* s = nullptr) {
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

	struct RIFFHeader
	{
		RIFFId IdRIFF; // 'RIFF'
		int szRIFF;
		RIFFId RiffFormat;  // 'WAVE' -- means two subchunks, 'fmt ' and 'data'

		RIFFId IdFmt; // 'fmt '
		int  szFmt;	// 16, 18 or 40
		short AudioFormat;
		short NumChannels;
		int SampleRate;
		int ByteRate;
		short BlockAlign;
		short BitsPerSample;

		RIFFHeader() : IdRIFF("RIFF"),
			szRIFF(0),
			RiffFormat("WAVE"),
			IdFmt("fmt "),
			szFmt(16),
			AudioFormat(1),
			NumChannels(1),
			SampleRate(16000),
			ByteRate(0),
			BlockAlign(0),
			BitsPerSample(16)
		{
		}

		explicit RIFFHeader(const char* bytes)
		{
			*this = *reinterpret_cast<RIFFHeader*>(const_cast<char*>(bytes));
		}
	};

	struct DATAChunk
	{
		RIFFId IdData;  // 'data'
		int	szData;
	};

#pragma pack(pop)

#if SUPPORTS_STEREO
	class Stereo16BitReader :
		public sel::eng::proc::scalar_stream_reader<SAMP16, 1024>
	{
	public:
		PIN default_outpin() const { return LEFT; }
		Stereo16BitReader(int sampleRate) :
			FixedSizePacketReader<StereoSamp<double>, 1024>(2, TIME_T(1.0 / sampleRate))
		{}

		void ProcessPacket(StereoSamp<double>& p)
		{
			out[0] = (SIGNAL_T)(p.L() / 32768.0);
			out[1] = (SIGNAL_T)(p.R() / 32768.0);
		}

		const char* name() const
		{
			return "16-bit Stereo RIFF File Reader";
		}

		virtual ~Stereo16BitReader() {}

	};
#endif
	template<size_t port_width>class Mono16BitReader :
		public sel::eng::proc::scalar_stream_reader<SAMP16, port_width>
	{
	public:
		Mono16BitReader(sel::eng::input_stream& stream, int samps_per_second) :
			sel::eng::proc::scalar_stream_reader<SAMP16, port_width>(stream, rate_t(samps_per_second, 1)) {}
	};

} // anonymous


namespace sel {
	namespace eng {
		namespace proc {

			template<size_t port_width> class wav_file_data_source : public scalar_stream_reader<SAMP16, port_width>, virtual public creatable<wav_file_data_source<port_width>>
			{

			private:
				using file_descriptor = int;
				file_descriptor fd_;

				file_input_stream fis;

				const char* filename_;

				int fs_;

				short numChannels;

				long datastart = 0;

				double totDuration;

				file_descriptor ParseHeader() {

					RIFFHeader header;
					DATAChunk data;

					file_descriptor fd = open(filename_, O_RDONLY + O_BINARY);

					try {
						if (-1 == fd)
							throw sys_ex();

						if (!read(fd, &header, sizeof(header)))
							throw sys_ex();


						if (header.IdRIFF != "RIFF")
							throw eng_ex("WAV File format is incorrect. (Missing 'RIFF' chunk).");

						if (header.RiffFormat != "WAVE")
							throw eng_ex("WAV File format is incorrect. (Format is not 'WAVE').");

						if (header.IdFmt != "fmt ")
							throw eng_ex("WAV File format is incorrect. (Missing 'fmt ' chunk).");



						if (header.AudioFormat != 1) // PCM
							throw eng_ex("The WAV file is not PCM format.");


						if (header.BitsPerSample != 16)

							throw eng_ex("Can only read 16 bit wav files.");

						this->fs_ = header.SampleRate;

						this->numChannels = header.NumChannels;
#if SUPPORTS_STEREO
						if this->numChannels > 2)
						throw eng_ex("WAV File has more than 2 audio channels.");
#else
						if (this->numChannels > 1)
							throw eng_ex("WAV File has more than 1 audio channel.");
#endif

						if (header.szFmt == 16) { // should be 'data' chunk next
							;
						}
						else {
							// scan to end of fmt chunk
							lseek(fd, header.szFmt - 16, SEEK_CUR);
						}
						// repeatedly next chunks into struct, assume it's a data chunk.
						do {
							if (!read(fd, &data, sizeof(data)))
								throw sys_ex();
							if (data.IdData == "data")
								break;
							lseek(fd, data.szData, SEEK_CUR); // not 'data', skip chunk

						} while (true); // skip non-data chunks




						if (data.IdData != "data")
							throw eng_ex("WAV File format is incorrect. (Missing 'data' chunk).");

						datastart = tell(fd);


						this->totDuration = (8 * (double)data.szData / (header.BitsPerSample * header.NumChannels)) / this->fs_;

					}
					catch (eng_ex& ex) {

						if (fd > 0) close(fd);
						throw ex;
					}

					return fd; // return file pointer, pointing at start of data
				}

			public:
				void process() final
				{
					scalar_stream_reader<SAMP16, port_width>::process();
					for (size_t i = 0; i < port_width; ++i)
						this->out[i] /= 32768.0;
				}
				virtual const std::string type() const override { return "wav file reader"; }
				wav_file_data_source() : scalar_stream_reader<SAMP16, port_width>(nullptr, rate_t()) {}

				wav_file_data_source(const uri& u) : scalar_stream_reader<SAMP16, port_width>(nullptr, rate_t()), fd_(0), filename_(nullptr)
				{

					u.assert_scheme(uri::FILE);
					filename_ = u.path();

				}
				wav_file_data_source(params& args) : wav_file_data_source(args.get<const char*>("uri"))
				{
				}

				void init(schedule* context)  final
				{
					this->set_stream(&fis);
					this->fd_ = ParseHeader();
					auto pos = tell(this->fd_);
					fis.attach(this->fd_);

					this->set_rate(rate_t(this->fs_, 1));

					if (numChannels == 1)
						;
					else
#if SUPPORTS_STEREO
						if (numChannels == 2)
							;
						else
#else
						throw eng_ex("Number of channels not supported");
#endif

				}

				void freeze() final
				{
					// Note: Need to parse wave header at freeze() time in order to figure out number of output ports to create
					// leaves file open if successful otherwise closes file and throws exception
					// this->fd_ = ParseHeader();


				}

			};
		}
	}
}
