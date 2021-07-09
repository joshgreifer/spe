#pragma once
#include "../processor.h"
#include "../array2d.h"
namespace sel {
    namespace eng6 {
        namespace proc {
            // Discrete Cosine Transform, types I, II, III, and IV

            // https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
            // This code follows SciPy's dct methods
            // https://docs.scipy.org/doc/scipy/reference/generated/scipy.fftpack.dct.html
            /// TODO: ortho-normalization


            template<class traits, size_t DctType=2U, size_t SZ = traits::input_frame_size>struct dct : public Processor1A1B<SZ, SZ>, virtual public creatable<dct<traits, DctType, SZ> >
            {
                static_assert(DctType >= 1 && DctType <= 4);
                static_assert(DctType != 1 || SZ > 1);

                static constexpr size_t N = SZ;
                using factors_t = sel::array2d<double, SZ, SZ>;
                mutable factors_t *factors_;
                factors_t& factors;

                void make_factors()
                {

                    for (size_t k = 0; k < N; ++k)
                        for (size_t n = 0; n < N; ++n)
                            if constexpr (DctType==1U)
                                factors(k, n) =  cos(M_PI / (N-1) * n * k);
                            else if constexpr (DctType==2U)
                                factors(k, n) =  cos(M_PI / N * (n + 0.5) * k);
                            else if constexpr (DctType==3U)
                                factors(k, n) =  cos(M_PI / N * n * (k + 0.5));
                            else if constexpr (DctType==4U)
                                factors(k, n) = cos(M_PI / N * (n + 0.5) * (k + 0.5));

                }
            public:
                virtual const std::string type() const override { return "discrete cosine transform"; }

                void process() final
                {
                    const auto first_x = this->in[0];
                    const auto last_x = this->in[N-1];
                    int flip = 1;
                    for (size_t k = 0; k < SZ; ++k) {

                        if constexpr (DctType==1) {
                            double sum = 0;
                            for (size_t n = 1; n < SZ - 1; ++n)
                                sum += this->in[n] * factors(k, n);
                            this->out[k] = first_x + last_x * flip + sum * 2.0;
                            flip = -flip;
                        } else if constexpr (DctType==2) {
                            double sum = 0;
                            for (size_t n = 0; n < SZ; ++n)
                                sum += this->in[n] * factors(k, n);
                            this->out[k] = sum * 2.0;
                        } else if constexpr (DctType==3) {
                            double sum = 0;
                            for (size_t n = 1; n < SZ; ++n)
                                sum += this->in[n] * factors(k, n);
                            this->out[k] = first_x + sum * 2.0;
                        } else if constexpr (DctType==4) {
                            double sum = 0;
                            for (size_t n = 0; n < SZ; ++n)
                                sum += this->in[n] * factors(k, n);
                            this->out[k] = sum * 2.0;
                        }
                    }

                }
                // default constructor needed for factory creation
                explicit dct() :  factors_(new factors_t), factors(*factors_) {
                    make_factors();
                }

                dct(params& args) : dct()
                {
                }

                ~dct()
                {
                    delete factors_;
                }

            };
        } // proc
    } // eng
} //sel


#if defined(COMPILE_UNIT_TESTS)
#pragma once
#include "../unit_test.h"

SEL_UNIT_TEST(dct)

        class ut_traits : public eng_traits<1024, 16000> {};



        using dct1 = sel::eng6::proc::dct<ut_traits, 1>;
        using dct2 = sel::eng6::proc::dct<ut_traits, 2>;
        using dct3 = sel::eng6::proc::dct<ut_traits, 3>;
        using dct4 = sel::eng6::proc::dct<ut_traits, 4>;
        using rng = sel::eng6::proc::rand<ut_traits::input_frame_size>;

        void run() {

            auto py_dct = python::get().scipy_fftpack.attr("dct");

            rng rng1;
            dct1 my_dct1;
            dct2 my_dct2;
            dct3 my_dct3;
            dct4 my_dct4;

            std::vector<sel::eng6::Processor1A1B<ut_traits::input_frame_size, ut_traits::input_frame_size>* > dcts = {
                    &my_dct1, &my_dct2, &my_dct3, &my_dct4
            };

            size_t dct_type = 0;
            rng1.process();
            const char *dct_type_names[] = { "n/a", "Type I", "Type II", "Type III", "Type IV" };
            for (auto pdct: dcts ) {
                ++dct_type; // iterates through 1,2,3,4
                SEL_UNIT_TEST_ITEM(dct_type_names[dct_type]);
                pdct->ConnectFrom(rng1);
                pdct->freeze();
                pdct->process();

                auto my_dct_result = pdct->Out(0)->as_vector();
                py::array_t<double> py_dct_result = py_dct(rng1.Out(0)->as_vector(), "type"_a = dct_type);
                auto py_dct_result_vec = python::make_vector_from_1d_numpy_array(py_dct_result);
                for (size_t i = 0; i < dct4::N; ++i)
                SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(my_dct_result[i],py_dct_result_vec[i]);
            }

        }

SEL_UNIT_TEST_END
#endif