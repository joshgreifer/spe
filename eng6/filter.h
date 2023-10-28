//
// Created by josh on 18/06/2022.
//
#include <vector>

namespace sel {
    namespace eng6 {

        template<size_t N>struct Coeffs {
            std::array<double, N> b;
            std::array<double, N> a;
        };

        using BiquadCoeffs=Coeffs<3>;


        // https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt
        namespace biquad {

            enum FilterType {
                LP, LPF = LP, LowPass = LP,
                HP, HPF = HP, HighPass = HPF,
                BP, BPF = BP, BandPass = BP,
                Notch,
                AP, APF = AP, AllPass = AP,
                PeakingEQ,
                LowShelf,
                HighShelf,

            };

            enum ParamType {
                Q,
                BW,
                S
            };

            BiquadCoeffs calc_coeffs(FilterType type, double Fs, double f0, double param, ParamType param_type,
                            double db_gain = DBL_MAX) {

                BiquadCoeffs coeffs;

                double &a0 = coeffs.a[0];
                double &a1 = coeffs.a[1];
                double &a2 = coeffs.a[2];
                double &b0 = coeffs.b[0];
                double &b1 = coeffs.b[1];
                double &b2 = coeffs.b[2];

                if (type == LowShelf || type == HighShelf) {
                    if (db_gain == DBL_MAX)
                        throw eng_ex("Must provide db_gain for shelving filters");
                } else if (param_type == S)
                    throw eng_ex("param_type S is only valid for shelving filters");


                const double A = (type == LowShelf || type == HighShelf || type == PeakingEQ) ?
                                 pow(10, (db_gain / 40)) : 0.0;
                const double w0 = 2 * M_PI * f0 / Fs;
                const double cos_w0 = cos(w0);
                const double sin_w0 = sin(w0);
                const double alpha =
                        param_type == Q ? sin_w0 / (2 * param) :
                        param_type == BW ? sin_w0 * sinh(log(2.0) / 2 * param * w0 / sin_w0) :
                        sin_w0 / 2 * sqrt((A + 1 / A) * (1.0 / param - 1) + 2);  // param_type == S

                switch (type) {
                    case LP:
                        b0 = (1 - cos_w0) / 2;
                        b1 = 1 - cos_w0;
                        b2 = (1 - cos_w0) / 2;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case HP:
                        b0 = (1 + cos_w0) / 2;
                        b1 = -(1 + cos_w0);
                        b2 = (1 + cos_w0) / 2;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case BP:
                        b0 = alpha;
                        b1 = 0;
                        b2 = -alpha;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case Notch:
                        b0 = 1;
                        b1 = -2 * cos_w0;
                        b2 = 1;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case AP:
                        b0 = 1 - alpha;
                        b1 = -2 * cos_w0;
                        b2 = 1 + alpha;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case PeakingEQ:
                        b0 = 1 + alpha * A;
                        b1 = -2 * cos_w0;
                        b2 = 1 - alpha * A;
                        a0 = 1 + alpha / A;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha / A;
                        break;
                    case LowShelf:
                    case HighShelf: {
                        const auto temp = 2 * sqrt(A) * alpha;
                        b0 = A * ((A + 1) - (A - 1) * cos_w0 + temp);
                        b1 = 2 * A * ((A - 1) - (A + 1) * cos_w0);
                        b2 = A * ((A + 1) - (A - 1) * cos_w0 - temp);
                        a0 = (A + 1) + (A - 1) * cos_w0 + temp;
                        a1 = -2 * ((A - 1) + (A + 1) * cos_w0);
                        a2 = (A + 1) + (A - 1) * cos_w0 - temp;
                        if (type == HighShelf) {
                            b1 = -b1;
                            a1 = -a1;
                        }
                    }
                        break;
                    default:
                        throw eng_ex("Unknown type");

                }
                return coeffs;

            }
        };

        template<class T>
        std::vector<T> poly_multiply(std::vector<T> A, std::vector<T> B) {
            const size_t m = A.size();
            const size_t n = B.size();

            std::vector<T> prod = std::vector<T>(m + n - 1);

            for (size_t i = 0; i < m; ++i)
                for (size_t j = 0; j < n; ++j)
                    prod[i + j] += A[i] * B[j];


            return prod;
        }

#ifndef SPE_FILTER_H
#define SPE_FILTER_H

#endif //SPE_FILTER_H
    }
}

#if defined(COMPILE_UNIT_TESTS)

#include "unit_test.h"
#include "procs/iir_filt.h"
SEL_UNIT_TEST(filter)

        struct ut_traits {
            static constexpr size_t signal_length = 10;
        };

        void run() {
            using  namespace sel::eng6::biquad;

            std::vector<int> u = {1, 0, 1};
            std::vector<int> v = {2, 7};
            std::vector<int> expected_w = {2, 7, 2, 7};
            std::vector<int> w = sel::eng6::poly_multiply(u, v);
            SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL_ARRAYS(w, expected_w);


            auto filt_lo = sel::eng6::proc::iir_filt<1>(calc_coeffs(LowShelf, 16000, 2000, .5, Q,  -3));
            auto filt_hi = sel::eng6::proc::iir_filt<1>(calc_coeffs(HighShelf, 16000, 3000, .5, Q,  -3));

            auto filt_band = filt_lo + filt_hi;

            auto foo = 1;

        }
SEL_UNIT_TEST_END
#endif



