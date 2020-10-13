#pragma once
#include <limits>
#include <complex>
#include <vector>
#include "Ratio.h"

//using namespace std;

typedef double samp_t;
typedef std::complex<samp_t> csamp_t;
typedef std::vector<samp_t> vector_t;
typedef std::vector<csamp_t> cvector_t;
static constexpr samp_t NO_SIGNAL = std::numeric_limits<samp_t>::quiet_NaN();

#undef min
#undef max
#undef free // conflicts with MSVC debug free
#define exprtk_enable_debugging
#define exprtk_disable_enhanced_features

#include "exprtk.hpp"


typedef exprtk::symbol_table<samp_t>	samp_symbol_table_t;
typedef exprtk::expression<samp_t>	samp_expression_t;
typedef exprtk::parser<samp_t>		samp_expr_parser_t;
typedef exprtk::parser_error::type	samp_expr_parser_error_t;

typedef sel::Ratio<size_t, 16> rate_t; 


template<size_t WINDOW_SIZE=256, size_t FS=16000>struct eng_traits
{
	static constexpr size_t input_frame_size = WINDOW_SIZE;
	static constexpr size_t hop_size = WINDOW_SIZE / 2; // 50% overlap
	static constexpr size_t input_fs = FS;
	static constexpr double sample_duration_secs = 1.0 / FS;
	static constexpr double window_duration_secs = WINDOW_SIZE * sample_duration_secs;
};
