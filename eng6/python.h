#pragma once
#if defined(COMPILE_WITH_PYTHON)
#include <vector>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "singleton.h"
namespace py = pybind11;
using namespace pybind11::literals; // to bring in the `_a` literal

class python : public sel::singleton<python>
{
	py::scoped_interpreter guard_ {};

public:
	template<class T> static std::vector<T>make_vector_from_1d_numpy_array(py::array_t<T>py_array)
	{
		return std::vector<T>(py_array.data(), py_array.data() + py_array.size());
	}

	py::module np;
	py::module librosa;
	py::module scipy_fftpack; // for DCT unit test
	python()
	{
		np = py::module::import("numpy");
		librosa = py::module::import("librosa");
        scipy_fftpack = py::module::import("scipy.fftpack");
	}
};
#endif