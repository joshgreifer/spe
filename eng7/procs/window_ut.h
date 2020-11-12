#pragma once

#include "../../eng6/unit_test.h"
#include <iostream>

// window unit test
#include "window.h"
//#include "wav_file_data_source.h"

#include "../../eng6/scheduler.h"

#include "../graph.h"

SEL_UNIT_TEST(window7)

struct ut_traits_no_overlap
{
	static constexpr size_t input_frame_size = 8;
	static constexpr size_t overlap = 0;
	static constexpr double kaiser_beta = 5.0;
};

struct ut_traits_overlap
{
	static constexpr size_t input_frame_size = 10;
	static constexpr size_t overlap = 3;
	static constexpr size_t iters = 10;
	static constexpr size_t input_fs = 16000;
};

struct ut_traits
{
	static constexpr size_t input_fs = 16000;
	static constexpr size_t output_fs = 16000;
	static constexpr size_t input_frame_size = 1024;
	static constexpr size_t hop_size = 256;
	static constexpr size_t overlap = input_frame_size - hop_size;

};
struct sig_gen_ramp : sel::eng6::Processor01A<ut_traits_overlap::input_frame_size>
{
	size_t c = 0;
	void process() final
	{
		for (size_t i = 0; i < ut_traits_overlap::input_frame_size; ++i)
			out[i] = static_cast<samp_t>(c++);
	}
} sig_gen_ramp;

using kaiser_window = sel::eng7::proc::window_t<ut_traits_no_overlap, sel::eng7::proc::wintype::KAISER<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using hamming_window = sel::eng7::proc::window_t<ut_traits_no_overlap, sel::eng7::proc::wintype::HAMMING<ut_traits_no_overlap>, ut_traits_no_overlap::input_frame_size>;
using rectangular_window = sel::eng7::proc::window_t<ut_traits_overlap, sel::eng7::proc::wintype::RECTANGULAR<ut_traits_overlap>, ut_traits_overlap::input_frame_size>;
using hann_window = sel::eng7::proc::window_t<ut_traits, sel::eng7::proc::wintype::HANN<ut_traits>, ut_traits::input_frame_size>;

//using wav_reader = sel::eng6::proc::wav_file_data_source<ut_traits::input_frame_size>;



static constexpr size_t N = ut_traits_no_overlap::input_frame_size;



struct nullsink : sel::eng7::stdsink<ut_traits_no_overlap::input_frame_size>
{
    void process() final {}
} nullsink1;


struct impulse : sel::eng7::data_source<N, samp_t> {


    impulse(std::initializer_list<samp_t> ilist) {
        auto v1 = std::vector<samp_t>(ilist);

        for (auto i = 0; i < N; ++i)
            this->out()[i] = v1[i];
        raise(2);
    }
    void process() final {
        if (this->count() == 0)
            throw std::error_code(eng_errc::input_stream_eof);
    }

    impulse(std::array<samp_t, N>&& v) {
        this->out() = v;
        raise();
    }
};
void run() {
    std::array<samp_t, N> matlab_kaiser_result = { {
                                                           0.0367108922712867,
                                                           0.270694417889416,
                                                           0.651738235245363,
                                                           0.955247316456436,
                                                           0.955247316456436,
                                                           0.651738235245363,
                                                           0.270694417889416,
                                                           0.0367108922712867
                                                   }
    };




    auto &s = sel::eng6::scheduler::get();
    sel::eng7::proc::processor_graph graph(s);

	kaiser_window kaiser_window1;

    impulse input({1, 1, 1, 1, 1, 1, 1, 1});
    graph.connect(input, kaiser_window1);
    graph.connect(kaiser_window1, nullsink1);


    s.run();
    std::array<samp_t, N>& my_kaiser_result(kaiser_window1.out());
	SEL_UNIT_TEST_ITEM("kaiser window");
	// compare matlab fft
	for (size_t i = 0; i < N; ++i) 
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(my_kaiser_result[i], matlab_kaiser_result[i]);


}

SEL_UNIT_TEST_END
