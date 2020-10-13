#pragma once
#if defined(COMPILE_UNIT_TESTS)
#include "singleton.h"

#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals; // to bring in the `_a` literal

class python : public sel::singleton<python>
{
	py::scoped_interpreter guard_ {};

public:
	py::module np;
	py::module librosa;
	python()
	{
		np = py::module::import("numpy");
		librosa = py::module::import("librosa");
	}
};
#endif