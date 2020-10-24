#pragma once
#include "../new_processor.h"
#include "fft_impl.h"
//template<size_t SZ>class sp_ac;
namespace sel {
	namespace eng7 {
		namespace proc {

			template<typename traits> struct fft_t : public stdproc<traits::input_frame_size, traits::input_frame_size, samp_t, csamp_t>, virtual public creatable<fft_t<traits> >
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



					// real input, convert to complex (via operator=())
					for (size_t i = 0; i < SZ; ++i)
						this->out()[i] = this->in_v()[i];

					gfft.fft(reinterpret_cast<samp_t *>(this->out().data()));

				}
				// default constuctor needed for factory creation
				explicit fft_t() {}

				fft_t(params& args)
				{
				}


			};
			template<typename traits> struct fftr_t : public stdproc<traits::input_frame_size, traits::input_frame_size / 2 + 1, samp_t, csamp_t>, virtual public creatable<fftr_t<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				friend class unit_test_fft;

				GFFT<SZ, samp_t, 1> gfft;

				// Unlike full fft, which transforms the output port 'in place',  real fft needs an internal buffer
				// This is because although we only have the output array size of N/2+1  but still need
				// the full N for the fft calculation itself.
				
				csamp_t complex_data[SZ];
				// 
				samp_t * const data_as_array_of_reals = reinterpret_cast<samp_t *>(complex_data.data());

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
						complex_data[i] = this->in_v()[i];

					gfft.fft(data_as_array_of_reals);

					// ignore conjugates
					for (size_t i = 0; i < 2 * (SZ / 2 + 1); ++i)
						this->out()[i] = complex_data[i];


				}
				// default constuctor needed for factory creation
				explicit fftr_t() {}

				fftr_t(params& args)
				{
				}


			};


			template<typename traits>struct ifft : public stdproc<traits::input_frame_size, traits::input_frame_size, csamp_t, csamp_t>, virtual public creatable<ifft<traits> >
			{
				static constexpr size_t SZ = traits::input_frame_size;

				//	friend class sp_ac<SZ>;
				friend class unit_test_fft;
				GFFT<SZ, samp_t, 1> gfft;


			public:
				virtual const std::string type() const override { return "ifft";  }
				void process() final
				{

					csamp_t *out_as_complex_array = (csamp_t *)this->out().data();
					// complex input, do conj and also copy  to out (which is changed in-place by fft)
					for (size_t i = 0; i < SZ; ++i) {
						out_as_complex_array[i] = std::conj(this->in_v()[i]);
					}
					gfft.fft(reinterpret_cast<samp_t *>(this->out().data()));

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
		} // proc
	} // eng
} // sel


#if defined(COMPILE_UNIT_TESTS)
#include "../../eng/unit_test.h"

SEL_UNIT_TEST(fft);

        struct ut_traits
        {
            static constexpr size_t input_frame_size = 1024;
        };
        using fft = sel::eng7::proc::fft_t<ut_traits>;
        using ifft = sel::eng7::proc::ifft<ut_traits>;


        static constexpr size_t  SZ = ut_traits::input_frame_size;

        void run() {
            const auto seed = 5489U;
            sel::eng7::proc::rand<SZ> rng(seed);


            fft fft1;
            rng.connect_to(fft1);

            rng.process();
            fft1.process();

            auto& my_fft_result = fft1.out();

            SEL_UNIT_TEST_ITEM("np.fft.fft");
            auto py_np_fft_fft = python::get().np.attr("fft").attr("fft");
            py::array_t<double> rand_py(SZ, rng.out().data());
            py::array_t<csamp_t> fft_py = py_np_fft_fft(rand_py);
            auto fft_py_vec = python::make_vector_from_1d_numpy_array(fft_py);
            for (size_t i = 0; i < SZ; ++i) {
                SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(fft_py_vec[i].real(), my_fft_result[i].real());
                SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(fft_py_vec[i].imag(), my_fft_result[i].imag());
            }
            ifft ifft1;
            fft1.connect_to(ifft1);
            ifft1.process();

            // compare to input
            SEL_UNIT_TEST_ITEM("invert (ifft)");
            for (size_t i = 0; i < SZ; ++i)
            SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(ifft1.out()[i].real(), rng.out()[i]);

        }

SEL_UNIT_TEST_END

#endif


