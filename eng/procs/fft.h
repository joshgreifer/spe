#pragma once
#include "../eng_traits.h"
#include "../processor.h"
#include "fft_impl.h"
//template<size_t SZ>class sp_ac;
namespace sel {
	namespace eng {
		namespace proc {

			template<typename traits> struct fft_t : public Processor1A1B<traits::input_frame_size, 2 * traits::input_frame_size>, virtual public creatable<fft_t<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				friend class unit_test_fft;

				GFFT<SZ, samp_t, 1> gfft;

			public:

				
				const std::string type() const final 
				{
					char buf[100];
					snprintf(buf, 100, "fft[%zd]", SZ);
					return buf;
				}
				

				void process(void)
				{
					csamp_t *out_as_complex_array = reinterpret_cast<csamp_t *>(this->out);
					const samp_t *in_array = this->in;

					// real input, convert to complex (via operator=())
					for (size_t i = 0; i < SZ; ++i)
						out_as_complex_array[i] = this->in[i];

					gfft.fft(this->out);

				}
				// default constuctor needed for factory creation
				explicit fft_t() {}

				fft_t(params& args)
				{
				}


			};
			template<typename traits> struct fftr_t : public Processor1A1B<traits::input_frame_size, traits::input_frame_size>, virtual public creatable<fftr_t<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				friend class unit_test_fft;

				GFFT<SZ, samp_t, 1> gfft;
				csamp_t complex_data[SZ];
				// for real fft, the first N complex values should be interpreted as reals
				samp_t * const real_data = reinterpret_cast<samp_t *>(&complex_data[0]);

			public:

				
				const std::string type() const final 
				{
					char buf[100];
					snprintf(buf, 100, "fftr[%zd]", SZ);
					return buf;
				}
				

				void process(void)
				{

					// real input, convert to complex (via operator=())
					for (size_t i = 0; i < SZ; ++i)
						complex_data[i] = this->in[i];

					gfft.fft(real_data);

					for (size_t i = 0; i < SZ; ++i)
						this->out[i] = real_data[i];


				}
				// default constuctor needed for factory creation
				explicit fftr_t() {}

				fftr_t(params& args)
				{
				}


			};


			template<typename traits = eng_traits<>>struct ifft : public Processor1A1B<2 * traits::input_frame_size, 2 * traits::input_frame_size>, virtual public creatable<ifft<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				//	friend class sp_ac<SZ>;
				friend class unit_test_fft;
				GFFT<SZ, samp_t, 1> gfft;


			public:
				virtual const std::string type() const override { return "ifft";  }
				void process() final
				{
					const csamp_t *in_as_complex_array = (csamp_t *)this->in;
					csamp_t *out_as_complex_array = (csamp_t *)this->out;
					// complex input, do conj and also copy  to out (which is changed in-place by fft)
					for (size_t i = 0; i < SZ; ++i) {
						out_as_complex_array[i] = std::conj(in_as_complex_array[i]);
					}
					gfft.fft(this->out);

					// conj again
					for (size_t i = 0; i < SZ; ++i) {
						out_as_complex_array[i] = std::conj(out_as_complex_array[i]) / (double)SZ;
					}

				}

				// default constuctor needed for factory creation
				explicit ifft() {}

				ifft(params& args)
				{
				}

			};
#if 0
			template<size_t SZ, size_t OUTSZ>class sp_ac : public  RegisterableSigProc<sp_ac<SZ, OUTSZ>, SZ, OUTSZ>
			{
				friend class unit_test_fft;

				sp_fft<SZ> fft;
				sp_ifft<SZ> ifft;

			public:
				PIN default_outpin() const { return ALL; }
				PIN default_inpin() const { return ALL; }

				void init(Schedule *context)
				{
					in_as_array(); // throws exception if inputs aren't an array
					ProxyInput(&fft);
					ifft.ConnectFrom(&fft, ALL, ALL);

				}

				void process(void)
				{
					fft.process();
					COMPLEX_T *fft_as_complex_array = (COMPLEX_T *)fft.out_as_array();
					COMPLEX_T *ifft_as_complex_array = (COMPLEX_T *)ifft.out_as_array();
					// multiply each point by its complex conjugate (will reduce imag to 0.0 if input is real)
					for (int i = 0; i < SZ; ++i)
						fft_as_complex_array[i] *= conj(fft_as_complex_array[i]);

					ifft.process();
					for (size_t i = 0; i < OUTSZ; ++i)
						out[i] = ifft_as_complex_array[i].real() / (SIGNAL_T)SZ;

				}
				// default constuctor needed for factory creation
				explicit sp_ac() {}

				sp_ac(params& args)
				{
				}


			};


			// https://uk.mathworks.com/help/signal/ug/power-spectral-density-estimates-using-fft.html
			// input is an fft 
			template<size_t SZ, uint FS>struct sp_psd : public  RegisterableSigProc<sp_psd<SZ, FS>, SZ * 2, SZ / 2 + 1>
			{
				friend class unit_test_fft;

			public:
				PIN default_outpin() const { return ALL; }
				PIN default_inpin() const { return ALL; }

				void init(Schedule *context)
				{
					in_as_array(); // throws exception if inputs aren't an array
				}

				void process(void)
				{
					const COMPLEX_T *in_as_complex_array = (COMPLEX_T *)in_as_array();

					for (int i = 0; i < SZ / 2 + 1; ++i) {
						SIGNAL_T v = abs(in_as_complex_array[i]);
						v = 1.0 / (FS * SZ) * v * v;
						if (i > 0 && i < SZ / 2)
							v *= 2;
						out[i] = v;

					}
				}

				// default constuctor needed for factory creation
				explicit sp_psd() {}

				sp_psd(params& args)
				{
				}


			};

			// The following proc should be used for testing sp_spectrogram
			// Generate a gradient test pattern for client apps
			template<size_t SZ, uint FS, uint FRAMES_PER_SECOND>struct sp_spectrogram_test_gradient : public  RegisterableSigProc<sp_spectrogram_test_gradient<SZ, FS, FRAMES_PER_SECOND>, SZ * 2, SZ / 2 + 1>
			{

				static constexpr size_t N_OUTPINS = SZ / 2 + 1;
			public:
				PIN default_outpin() const { return ALL; }
				PIN default_inpin() const { return ALL; }

				void init(Schedule *context)
				{
					in_as_array(); // throws exception if inputs aren't an array
				}

				void process(void)
				{
					static int c;
					static bool reverse = false;
					// vertical line every second
					if (++c == FRAMES_PER_SECOND) {
						c = 0;
						reverse = !reverse;
						for (int i = 0; i < N_OUTPINS; ++i) out[i] = .8;
					}
					else  // draw a gradient
						if (reverse)
							for (int i = 0; i < N_OUTPINS; ++i) out[i] = ((SIGNAL_T)N_OUTPINS - i - 1) / (SIGNAL_T)N_OUTPINS;
						else
							for (int i = 0; i < N_OUTPINS; ++i) out[i] = i / (SIGNAL_T)N_OUTPINS;


				}

				// default constuctor needed for factory creation
				explicit sp_spectrogram_test_gradient() {}

				sp_spectrogram_test_gradient(params& args)
				{
				}


			};
			// https://uk.mathworks.com/help/signal/ug/power-spectral-density-estimates-using-fft.html
			// input is an fft 
			// do power spectral density, scale to decibels,  make range 0.0 .. 1.0

			template<size_t SZ, uint FS>struct sp_spectrogram : public  RegisterableSigProc<sp_spectrogram<SZ, FS>, SZ * 2, SZ / 2 + 1>,
				public iRunTimeInspectable,
				public iRunTimeModifiable
			{

				static constexpr size_t N_OUTPINS = SZ / 2 + 1;

				SIGNAL_T min_dB;
				SIGNAL_T max_dB;
				SIGNAL_T max_freq; // freq with most power

			public:
				PIN default_outpin() const { return ALL; }
				PIN default_inpin() const { return ALL; }

				void init(Schedule *context)
				{
					in_as_array(); // throws exception if inputs aren't an array
					max_freq = 0.0;

				}

				void process(void)
				{
					const COMPLEX_T *in_as_complex_array = (COMPLEX_T *)in_as_array();

					SIGNAL_T min = min_dB;
					SIGNAL_T max = max_dB;
					SIGNAL_T max_freq_bin = -1;

					for (int i = 0; i < N_OUTPINS; ++i) {
						SIGNAL_T v = abs(in_as_complex_array[i]);
						v = 1.0 / (FS * SZ) * v * v;
						if (i > 0 && i < SZ / 2)
							v *= 2;
						SIGNAL_T temp = 10.0 * log10(v);
						out[i] = temp;
						if (temp > max) { max = temp; max_freq_bin = i; }
						if (temp < min) min = temp;
					}
					// normalize to max
					for (int i = 0; i < N_OUTPINS; ++i) {
						out[i] -= min;
						out[i] /= (max - min);
					}
					if (min_dB > min) min_dB = min;
					if (max_dB < max) {
						max_dB = max;
						max_freq = max_freq_bin / SZ * FS;
					}

				}
				void Modify(params& args)
				{
					max_dB = opt_arg<double>(args, "max", max_dB);
					min_dB = opt_arg<double>(args, "min", min_dB);
				}

				std::string Inspect(const char *attribute) const
				{
					char buf[64];
					if (!_stricmp(attribute, "max")) {
						sprintf_s(buf, 64, "%f", max_dB);
						return buf;
					}
					else if (!_stricmp(attribute, "max_freq")) {
						sprintf_s(buf, 64, "%f", max_freq);
						return buf;

					}
					else if (!_stricmp(attribute, "min")) {
						sprintf_s(buf, 64, "%f", min_dB);
						return buf;
					}
					// else
					throw eng_ex(UNKNOWN_INSPECT_ATTRIBUTE, "Inspectable attributes for this object: 'max, max_freq, min'");
				}
				// default constuctor needed for factory creation
				explicit sp_spectrogram() {}

				sp_spectrogram(params& args)
				{
					max_dB = opt_arg<double>(args, "max", -DBL_MAX);
					min_dB = opt_arg<double>(args, "min", DBL_MAX);
				}
			};

			/*
			fft unit test
			*/

			class unit_test_fft
			{
				static constexpr size_t SZ = 64;

				std::array<SIGNAL_T, SZ> input
				{ {
						-4911.43080257359, -4648.75282177202, -2049.90893844119, -1941.83692490218,
						-1757.64936335238, -804.438841549709, -735.172838921937, -554.898542043838,
						-542.549799169880, -517.934644663490, -456.347218936102, -424.653394382480,
						-352.079161485175, -343.446646570997, -306.270240885125, -303.642673849313,
						-301.209207465972, -288.778228758572, -230.728361813264, -217.743527148850,
						-194.634938699500, -169.166329820594, -168.551445313991, -153.723394003659,
						-151.230944621563, -146.780147037939, -135.148833700298, -126.693676935488,
						-125.552098064491, -122.704445307867, -120.582494617052, -118.740309356804,
						-117.746385186952, -117.218568438222, -116.583578868798, -110.799421039188,
						-106.892373019838, -105.603222157213, -100.079692464756, -96.1833578542874,
						-94.7379934704439, -93.2463242585609, -93.0786685060479, -92.7785224714423,
						-90.8551296913823, -90.5138921710392, -88.6267757962204, -86.8779026580548,
						-84.2174719564973, -83.5656659635882, -80.0311450839270, -79.2560931323279,
						-75.9927469681787, -75.4177304654045, -75.2733826216122, -74.9360776766520,
						-74.4290610751853, -69.1942449581439, -68.9462387427999, -68.7165676256769,
						-67.8152204254259, -66.9516267299665, -64.2061183905124, -64.1172832722024
					} };

				std::array<COMPLEX_T, SZ> matlab_fft_result = { {
						COMPLEX_T(-26197.8697193059, 0.00000000000000),
						COMPLEX_T(-16439.3509432869, -6364.65358836188),
						COMPLEX_T(-12767.1553141995, -7002.93389651327),
						COMPLEX_T(-10816.5394048286, -6722.52404180729),
						COMPLEX_T(-9540.50976596924, -6840.69173047631),
						COMPLEX_T(-8078.77971448977, -6790.91585371869),
						COMPLEX_T(-6929.24134517610, -6472.68374339679),
						COMPLEX_T(-6091.67959448038, -6003.24780745581),
						COMPLEX_T(-5328.90951393644, -5670.81561555180),
						COMPLEX_T(-4589.58675055666, -5091.11487328737),
						COMPLEX_T(-4223.69928206566, -4359.94367754186),
						COMPLEX_T(-4175.24742196536, -3831.27816530205),
						COMPLEX_T(-4196.87530377339, -3509.57008364077),
						COMPLEX_T(-4215.34996926699, -3307.17591547347),
						COMPLEX_T(-4215.66526513541, -3179.94003840826),
						COMPLEX_T(-4244.57812772813, -3128.38291131501),
						COMPLEX_T(-4159.48672412282, -3228.11571227089),
						COMPLEX_T(-3966.20597148335, -3276.49133954968),
						COMPLEX_T(-3743.95743314968, -3298.24086722008),
						COMPLEX_T(-3507.82959626911, -3329.16957185811),
						COMPLEX_T(-3174.95954185510, -3363.33530235525),
						COMPLEX_T(-2745.33861283343, -3293.37892685970),
						COMPLEX_T(-2357.82907506935, -3043.55069940990),
						COMPLEX_T(-2051.93474500213, -2824.44529578492),
						COMPLEX_T(-1683.25175369098, -2526.79562538936),
						COMPLEX_T(-1385.62156709758, -2069.32505435811),
						COMPLEX_T(-1261.84459050205, -1588.65018252464),
						COMPLEX_T(-1214.42154964770, -1136.58523878704),
						COMPLEX_T(-1294.27966378602, -698.985699697075),
						COMPLEX_T(-1497.22001505677, -396.357028426384),
						COMPLEX_T(-1650.61176140684, -260.571847036304),
						COMPLEX_T(-1679.26669419331, -141.911779573440),
						COMPLEX_T(-1679.24762135432, 0.00000000000000),
						COMPLEX_T(-1679.26669419331, 141.911779573440),
						COMPLEX_T(-1650.61176140684, 260.571847036304),
						COMPLEX_T(-1497.22001505677, 396.357028426384),
						COMPLEX_T(-1294.27966378602, 698.985699697075),
						COMPLEX_T(-1214.42154964770, 1136.58523878704),
						COMPLEX_T(-1261.84459050205, 1588.65018252464),
						COMPLEX_T(-1385.62156709758, 2069.32505435811),
						COMPLEX_T(-1683.25175369098, 2526.79562538936),
						COMPLEX_T(-2051.93474500213, 2824.44529578492),
						COMPLEX_T(-2357.82907506935, 3043.55069940990),
						COMPLEX_T(-2745.33861283343, 3293.37892685970),
						COMPLEX_T(-3174.95954185510, 3363.33530235525),
						COMPLEX_T(-3507.82959626911, 3329.16957185811),
						COMPLEX_T(-3743.95743314968, 3298.24086722008),
						COMPLEX_T(-3966.20597148335, 3276.49133954968),
						COMPLEX_T(-4159.48672412282, 3228.11571227089),
						COMPLEX_T(-4244.57812772813, 3128.38291131501),
						COMPLEX_T(-4215.66526513541, 3179.94003840826),
						COMPLEX_T(-4215.34996926699, 3307.17591547347),
						COMPLEX_T(-4196.87530377339, 3509.57008364077),
						COMPLEX_T(-4175.24742196536, 3831.27816530205),
						COMPLEX_T(-4223.69928206566, 4359.94367754186),
						COMPLEX_T(-4589.58675055666, 5091.11487328737),
						COMPLEX_T(-5328.90951393644, 5670.81561555180),
						COMPLEX_T(-6091.67959448038, 6003.24780745581),
						COMPLEX_T(-6929.24134517610, 6472.68374339679),
						COMPLEX_T(-8078.77971448977, 6790.91585371869),
						COMPLEX_T(-9540.50976596924, 6840.69173047631),
						COMPLEX_T(-10816.5394048286, 6722.52404180729),
						COMPLEX_T(-12767.1553141995, 7002.93389651327),
						COMPLEX_T(-16439.3509432869, 6364.65358836188)
					}
				};

				std::array<SIGNAL_T, SZ / 2 + 1> matlab_psd_result = { {
						670.242556474339,
						606.955224094522,
						414.143238234035,
						316.777058962669,
						269.172636613206,
						217.545351186600,
						175.605508714318,
						142.866297889710,
						118.272317096306,
						91.7651507693226,
						71.9702040950375,
						62.7175457315403,
						58.4586806394712,
						56.0675544907948,
						54.4606489756662,
						54.3031701605388,
						54.1446501171176,
						51.6917681765122,
						48.6242384364012,
						45.6801533489208,
						41.7820165009488,
						35.9047438574175,
						28.9503101701580,
						23.8045457551338,
						18.0039699195128,
						12.1133853667972,
						8.03918197854466,
						5.40360489311977,
						4.22605636029506,
						4.68509114740154,
						5.45393920773927,
						5.54702262384500,
						2.75378181037514
					} };

			public:
				unit_test_fft() {

					sp_fft<SZ> sp_fft;
					sp_fft.ConnectFrom(input);
					sp_fft.process();

					COMPLEX_T *my_fft_result = (COMPLEX_T *)sp_fft.out;

					// compare matlab fft
					for (size_t i = 0; i < SZ; ++i) {
						SIGNAL_T e = abs(my_fft_result[i] - matlab_fft_result[i]);
						//if (e >= 1e-10)
						assert(e < 1e-10);

					}


					sp_ifft<SZ> sp_ifft;
					sp_ifft.ConnectFrom(&sp_fft, ALL, ALL);
					sp_ifft.process();

					COMPLEX_T *my_ifft_result = (COMPLEX_T *)sp_ifft.out;

					// compare to input
					for (size_t i = 0; i < SZ; ++i) {
						SIGNAL_T e = abs(my_ifft_result[i] - input[i]);
						//if (e >= 1e-10)
						assert(e < 1e-10);

					}

					sp_psd<SZ, 16000> sp_psd;
					sp_psd.ConnectFrom(&sp_fft, ALL, ALL);
					sp_psd.process();

					SIGNAL_T *my_psd_result = sp_psd.out;

					// compare matlab psd
					for (size_t i = 0; i < SZ / 2 + 1; ++i) {
						SIGNAL_T e = abs(my_psd_result[i] - matlab_psd_result[i]);
						//if (e >= 1e-10)
						assert(e < 1e-10);

					}


				}


			};
#endif // 0
		} // proc
	} // eng
} // sel


#if defined(COMPILE_UNIT_TESTS)
#include "fft_ut.h"
#endif


