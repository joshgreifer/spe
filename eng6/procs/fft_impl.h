/***************************************************************************
    Copyright (C) 2006 by Volodymyr Myrnyy (Vladimir Mirnyi)
 ***************************************************************************
 Permission to use, copy, modify, distribute and sell this software for any
 purpose is hereby granted without fee, provided that the above copyright
 notice appear in all copies and that both that copyright notice and this
 permission notice appear in supporting documentation.
 ***************************************************************************

 Generic simple and efficient Fast Fourier Transform (FFT) implementation
 using template metaprogramming

 ***************************************************************************/

#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;


////// template class SinCosSeries
// common series to compile-time calculation
// of sine and cosine functions

template<unsigned M, unsigned N, unsigned B, unsigned A>
struct SinCosSeries {
    static constexpr double value() {
        return 1 - (A * M_PI / B) * (A * M_PI / B) / M / (M + 1)
                   * SinCosSeries<M + 2, N, B, A>::value();
    }
};


template<unsigned N, unsigned B, unsigned A>
struct SinCosSeries<N, N, B, A> {
    static constexpr double value() { return 1.; }
};

////// template class Sin
// compile-time calculation of sin(A*M_PI/B) function

template<unsigned B, unsigned A, typename T=double>
struct Sin;

template<unsigned B, unsigned A>
struct Sin<B, A, float> {
    static constexpr float value() {
        return (A * M_PI / B) * SinCosSeries<2, 24, B, A>::value();
    }
};

template<unsigned B, unsigned A>
struct Sin<B, A, double> {
    static constexpr double value() {
        return (A * M_PI / B) * SinCosSeries<2, 34, B, A>::value();
    }
};

////// template class Cos
// compile-time calculation of cos(A*M_PI/B) function

template<unsigned B, unsigned A, typename T=double>
struct Cos;

template<unsigned B, unsigned A>
struct Cos<B, A, float> {
    static constexpr float value() {
        return SinCosSeries<1, 23, B, A>::value();
    }
};

template<unsigned B, unsigned A>
struct Cos<B, A, double> {
    static constexpr double value() {
        return SinCosSeries<1, 33, B, A>::value();
    }
};


////// template class DanielsonLanczos
// Danielson-Lanczos section of the FFT

template<unsigned N, typename T=double, int SIGN = 1>
class DanielsonLanczos {
    DanielsonLanczos<N / 2, T, SIGN> next;
    T coeffs[N];

    static constexpr T wpr = -2.0 * SIGN * Sin<N, 1, T>::value() * SIGN * Sin<N, 1, T>::value();
    static constexpr T wpi = -SIGN * Sin<N, 2, T>::value();
public:
    DanielsonLanczos() {
        T wr = 1.0;
        T wi = 0.0;
        coeffs[0] = wr;
        coeffs[1] = wi;
        for (unsigned i = 2; i < N; i+=2) {
            const auto temp = wr;
            wr += wr * wpr - wi * wpi;
            wi += wi * wpr + temp * wpi;
            coeffs[i] = wr;
            coeffs[i + 1] = wi;
        }
    }


    void apply(T *data) {
        T *dataN = data + N;
        next.apply(data);
        next.apply(dataN);



//    Change dynamic calculation to the static one
//      wtemp = sin(M_PI/N);
//        wtemp = SIGN * Sin<N, 1, T>::value();
//        wpr = -2.0 * wtemp * wtemp;
//      wpi = -sin(2*M_PI/N);

//        T wr = 1.0;
//        T wi = 0.0;
        for (unsigned i = 0; i < N;) {

            const auto tempr = dataN[i] * coeffs[i] - dataN[i + 1] * coeffs[i + 1];
            const auto tempi = dataN[i] * coeffs[i + 1] + dataN[i + 1] * coeffs[i];
            dataN[i] = data[i] - tempr;
            dataN[i + 1] = data[i + 1] - tempi;
            data[i++] += tempr;
            data[i++] += tempi;

//            const auto temp = wr;
//            wr += wr * wpr - wi * wpi;
//            wi += wi * wpr + temp * wpi;
        }
    }
};


template<typename T, int SIGN>
class DanielsonLanczos<4,T, SIGN> {
public:
   void apply(T* data) {
      T tr = data[2];
      T ti = data[3];
      data[2] = data[0]-tr;
      data[3] = data[1]-ti;
      data[0] += tr;
      data[1] += ti;

      tr = data[6];
      ti = data[7];
      data[6] = data[5]-ti;
      data[7] = tr-data[4];
      data[4] += tr;
      data[5] += ti;

      tr = data[4];
      ti = data[5];
      data[4] = data[0]-tr;
      data[5] = data[1]-ti;
      data[0] += tr;
      data[1] += ti;

      tr = data[6];
      ti = data[7];
      data[6] = data[2]-tr;
      data[7] = data[3]-ti;
      data[2] += tr;
      data[3] += ti;
   }
};


template<typename T, int SIGN>
class DanielsonLanczos<2, T, SIGN> {
public:
    static void apply(T *data) {
        twiddle(data, data+2);
    }
    static void twiddle(std::complex<T>&a, std::complex<T>&b) {
        auto temp = b;
        b = a - temp;
        a += temp;
    }
    static void twiddle(T *a, T *b) {
        twiddle(reinterpret_cast< std::complex<T> &>(a[0]), reinterpret_cast< std::complex<T> &>(b[0]));
    }

};



////// template class GFFT
// generic fast Fourier transform main class

template<unsigned N, typename T=double, int SIGN = 1>
class GFFT {
//   enum { N = 1<<P };
    DanielsonLanczos<N, T, SIGN> recursion;

    void scramble(T *data) {
        unsigned i, m, j = 1;
        for (i = 1; i < 2 * N; i += 2) {
            if (j > i) {
                swap(data[j - 1], data[i - 1]);
                swap(data[j], data[i]);
            }
            m = N;
            while (m >= 2 && j > m) {
                j -= m;
                m >>= 1;
            }
            j += m;
        }
    }

public:
    void fft(T *data) {
        scramble(data);
        recursion.apply(data);
    }

};



/*
int main()
{
// range of the needed GFFT classes
    const unsigned Min = 1;
    const unsigned Max = 27;

// initialization of the object factory
    Loki::Factory<AbstractFFT<Tp>,unsigned int> gfft_factory;
    FactoryInit<GFFTList<GFFT,Min,Max>::Result>::apply(gfft_factory);

    unsigned long i;

// runtime definition of the data length
    int p = 2;

// create an instance of the GFFT
    AbstractFFT<Tp>* gfft = gfft_factory.CreateObject(p);

// sample data
    unsigned long n = 1<<p;
    Tp* data = new Tp [2*n];
    for (i=0; i<n; ++i) {
        data[2*i] = 2*i;
        data[2*i+1] = 2*i+1;
    }

// compute FFT
    gfft->fft(data);

// print the results
    cout<<"--------------------------------" << endl;
      for (i=0; i<n; ++i)
        cout << setw(10) << setprecision(5) << data[2*i] << "\t"
             << data[2*i+1] << "I" << endl;

    delete [] data;

    return 0;
}
*/