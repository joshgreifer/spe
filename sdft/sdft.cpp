//
// Created by josh on 06/10/2021.
//
using samp_t = double;
#include <complex>
#include <iostream>
#include <time.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <float.h>

#define DO_FFTW // libfftw
#define DO_SDFT
#define DO_GFFT

#include "../eng6/idx.h"
#if defined(DO_FFTW)
namespace fftw {
#include <fftw3.h>
}


fftw::fftw_plan plan_fwd;
fftw::fftw_plan plan_inv;

#endif

typedef std::complex<samp_t> complex_t;

// Buffer size, make it a power of two if you want to improve fftw
constexpr int N = 256;

#if defined(DO_GFFT)
#include "../eng6/procs/fft_impl.h"
GFFT<N, samp_t, 1> gfft;
#endif
// input signal
complex_t in[N];

// frequencies of input signal after ft
// Size increased by one because the optimized sdft code writes data to freqs[N]
complex_t freqs[N];

// output signal after inverse ft of freqs
complex_t out1[N];
complex_t out2[N];

// forward coeffs -2 PI e^iw -- normalized (divided by N)
complex_t coeffs[N];
// inverse coeffs 2 PI e^iw
complex_t icoeffs[N];

complex_t slow_coeffs[N][N];

// global index for input and output signals
sel::idx<N> idx;

// these are just there to optimize (get rid of index lookups in sdft)
complex_t oldest_data, newest_data;

//initialize e-to-the-i-thetas for theta = 0..2PI in increments of 1/N
void init_coeffs()
{
    for (int i = 0; i < N; ++i) {
        const samp_t a = -2.0 * M_PI * i  / samp_t(N);
        const auto c = cos(a);
        const auto s = sin(a);
        coeffs[i] = complex_t(c, s);
        icoeffs[i] = complex_t(c, -s);
    }


    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            samp_t a = -2.0 * M_PI * i * j / samp_t(N);
            slow_coeffs[i][j] = complex_t(cos(a)/* / N */, sin(a) /* / N */);
        }
    }
}


// initialize all data buffers
void init()
{
    // clear data
    for (int i = 0; i < N; ++i)
        in[i] = 0;
    // seed rand()
    srand(857);
    init_coeffs();
    oldest_data = newest_data = 0.0;
    idx.reset();
}

// simulating adding data to circular buffer
void add_data()
{
    oldest_data = in[idx];
    newest_data = in[idx] = complex_t(rand() / (samp_t)RAND_MAX);

}


// sliding dft
void sdft()
{
    const complex_t delta = newest_data - oldest_data;
    sel::idx<N> ci;
    for (auto &f: freqs) {
        f += delta * coeffs[ci];
        ci += idx;
    }
}

// sliding inverse dft
void isdft()
{
    complex_t delta = newest_data - oldest_data;
    sel::idx<N> ci;
    for (int i = 0; i < N; ++i) {
        freqs[i] += delta * icoeffs[ci];
        ci += idx;
    }
}

// "textbook" slow dft, nested loops, O(N*N)
void ft()
{
    for (int i = 0; i < N; ++i) {
        freqs[i] = 0.0;
        for (int j = 0; j < N; ++j) {
            freqs[i] += in[j] * slow_coeffs[i][j];
        }
    }
}

samp_t mag(complex_t& c)
{
    return sqrt(c.real() * c.real() + c.imag() * c.imag());
}

void powr_spectrum(samp_t *powr)
{
    for (int i = 0; i < N/2; ++i) {
        powr[i] = mag(freqs[i]);
    }

}

int main(int argc, char *argv[])
{
    const int NSAMPS = N;
    clock_t start, finish;

#if defined(DO_SDFT)
// ------------------------------ SDFT ---------------------------------------------
    init();

    start = clock();
    for (int i = 0; i < NSAMPS; ++i) {

        add_data();

        sdft();
//        ft();
        // Mess about with freqs[] here
        //isdft();

        ++idx; // bump global index

        if ((i % 1000) == 0)
            std::cerr << i << " iters..." << '\r';
    }
    finish = clock();

    std::cout << "\nSDFT: " << NSAMPS / ((finish-start) / (samp_t)CLOCKS_PER_SEC) << " fts per second." << std::endl;

    samp_t powr_sdft[N / 2];
    powr_spectrum(powr_sdft);
#endif

#if defined(DO_FFTW)

// ------------------------------ FFTW ---------------------------------------------
    plan_fwd = fftw::fftw_plan_dft_1d(N, (fftw::fftw_complex *)in, (fftw::fftw_complex *)freqs, FFTW_FORWARD, FFTW_MEASURE);
    plan_inv = fftw::fftw_plan_dft_1d(N, (fftw::fftw_complex *)freqs, (fftw::fftw_complex *)out2, FFTW_BACKWARD, FFTW_MEASURE);

    init();

    start = clock();
    for (int i = 0; i < NSAMPS; ++i) {

        add_data();

        fftw::fftw_execute(plan_fwd);
        // mess about with freqs here
        //fftw::fftw_execute(plan_inv);

        ++idx; // bump global index

        if ((i % 1000) == 0)
            std::cerr << i << " iters..." << '\r';
    }
    // normalize fftw's output
    for (int j = 0; j < N; ++j)
        out2[j] /= N;

    finish = clock();

    std::cout << "\nFFTW: " << NSAMPS / ((finish-start) / (samp_t)CLOCKS_PER_SEC) << " fts per second." << std::endl;
    fftw::fftw_destroy_plan(plan_fwd);
    fftw::fftw_destroy_plan(plan_inv);

    samp_t powr_fftw[N / 2];
    powr_spectrum(powr_fftw);
#endif

#if defined(DO_GFFT)
    init();

    start = clock();
    for (int i = 0; i < NSAMPS; ++i) {

        add_data();
        // gfft.fft is in-place
        for (size_t i  = 0; i < N; ++i)
            freqs[i] = in[i];
        gfft.fft(reinterpret_cast<samp_t *>(freqs));

        // Mess about with freqs[] here
        //isdft();

        ++idx; // bump global index

        if ((i % 1000) == 0)
            std::cerr << i << " iters..." << '\r';
    }
    finish = clock();

    std::cout << "\nGFFT: " << NSAMPS / ((finish-start) / (samp_t)CLOCKS_PER_SEC) << " fts per second." << std::endl;

    samp_t powr_gfft[N/2];
    powr_spectrum(powr_gfft);

#endif
const samp_t MAX_PERMISSIBLE_DIFF = 1e-12; // DBL_EPSILON;
samp_t diff;
#if defined(DO_SDFT) && defined(DO_FFTW)
// ------------------------------      ---------------------------------------------

    // check sdft gives same power spectrum as FFTW
    for (int i = 0; i < N/2; ++i)
        if ((diff = abs(powr_sdft[i] - powr_fftw[i])) > MAX_PERMISSIBLE_DIFF)
            printf("SDFT: Values differ by more than %g at index %d.  Diff = %g\n", MAX_PERMISSIBLE_DIFF, i, diff);

#endif
#if defined(DO_GFFT) && defined(DO_FFTW)
// ------------------------------      ---------------------------------------------

    // check gfft gives same power spectrum as FFTW
    for (int i = 0; i < N/2; ++i)
        if ((diff = abs(powr_gfft[i] - powr_fftw[i])) > MAX_PERMISSIBLE_DIFF)
            printf("GFFT: Values differ by more than %g at index %d.  Diff = %g\n", MAX_PERMISSIBLE_DIFF, i, diff);

#endif
        return 0;
}