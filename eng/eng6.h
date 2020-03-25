#pragma once

#include <cstring>
#include <typeinfo>
#include "object.h"
#include "func.h"
#include "params.h"
#include "factory.h"
#include "scheduler.h"
#include "file_input_stream.h"
#include "websocket_stream.h"
#include "procs/data_source.h"
#include "procs/compound_processor.h"
#include "procs/wav_file_data_source.h"
#include "procs/matlab_file_output.h"
#include "procs/mux_demux.h"
#include "procs/resampler.h"

#include "procs/expr.h"
#include "procs/running_stats.h"

#include "procs/window.h"

#include "procs/fir_filt.h"
#include "procs/iir_filt.h"
#include "procs/lattice_filter.h"

#include "procs/fft.h"
#include "procs/ac.h"
#include "procs/psd.h"
#include "procs/mfcc.h"
#include "procs/lpc.h"
#include "procs/dnn.h"
#include "procs/ewma.h"

#include "procs/rand.h"
#include "procs/samples.h"

#include "xml_loader.h"
#include "eng_traits.h"
