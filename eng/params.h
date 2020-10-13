#pragma once
#include <stdint.h>
#include "dictionary.h"
#include "Ratio.h"
#include <vector>
#include <initializer_list>

#include "eng_traits.h"
#include "pugixml.hpp"
using namespace pugi;

namespace sel {

	class params : public case_insensitive_dictionary<const char *>
	{
	public:
		params() {}

		// create params from c-style command line args, null terminated
		params(char **argv) 
		{
			for (;;) {
				const char *key = *argv++;
				if (!key)
					return;
				const char *value = *argv++;
				if (!key) // not key-value pair
					return;
				(*this)[key] = value;
			}
		}

		params(std::vector<const char *>&v)
		{
			const size_t nparams = v.size() & ~1;
			for (size_t i = 0; i < nparams; i+=2)
				(*this)[v[i]] = v[i+1];
			
		}

		params(std::initializer_list<const char *>ilist)
		{
			std::vector<const char *>v(ilist);
			const size_t nparams = v.size() & ~1;
			for (size_t i = 0; i < nparams; i += 2)
				(*this)[v[i]] = v[i + 1];

		}

		// construct params from XML attributes
		params(const xml_node& node_with_attributes) {

			for (auto a : node_with_attributes.attributes())
				(*this)[a.name()] = a.value();

		}

		template<class T>T get(const char *key) const {
			T retval;
			convertarg(this->at(key), retval);
			return retval;
		}

		template<class T>T get(const char *key, T defaultvalue) const {
			try {
				T retval;
				convertarg(this->at(key), retval);
				return retval;
			}
			catch (std::out_of_range&) {
				return defaultvalue;
			}
		}

		template<class T>static  T convert(const char *val) {
			T retval;
			convertarg(val, retval);
			return retval;
		}


	private:
		// TODO: Use exprtk
		static double expr2double(const char *value_as_string) { return 0.0; }

		template<class T>static void convertarg(const char *value_as_string, T&value)
		{
			value = T(value_as_string);
		}


		static void convertarg(const char *value, double& ret)
		{
			char *endptr;
			ret = (double)strtol(value, &endptr, 0);
			if (endptr < value + strlen(value)) {// can't fully convert
				ret = strtod(value, &endptr);
				if (endptr < value + strlen(value)) // can't fully convert
					ret = expr2double(value);
			}
		}

		static void convertarg(const char *value, int& ret)
		{
			char *endptr;
			ret = strtol(value, &endptr, 0);
			if (endptr < value + strlen(value)) // can't fully convert
				ret = (int)expr2double(value);
		}

		static void convertarg(const char *value, unsigned int& ret)
		{
			char *endptr;
			ret = strtoul(value, &endptr, 0);
			if (endptr < value + strlen(value)) // can't fully convert
				ret = (unsigned int)expr2double(value);
		}

		static void convertarg(const char *value, size_t& ret)
		{
			char *endptr;
			ret = (size_t)strtoul(value, &endptr, 0);
			if (endptr < value + strlen(value)) // can't fully convert
				ret = (size_t)expr2double(value);
		}

		static void convertarg(const char *value, bool& ret)
		{
			if (!_stricmp(value, "true") || !_stricmp(value, "on") || !_stricmp(value, "yes") || atoi(value))
				ret = true;
			else
				ret = false;
		}

		// delimited list of double-precision numbers
		static void convertarg(const char *value, std::vector<double>& ret)
		{
			char *p = const_cast<char *>(value);
			for (;;) {
				ret.push_back(strtod(p, &p));
				if (*p)
					++p;
				else
					break;
			}
		}

		// delimited list of integers, can use hex/octal
		static void convertarg(const char *value, std::vector<int>& ret)
		{
			char *p = const_cast<char *>(value);
			for (;;) {
				ret.push_back(strtol(p, &p, 0));
				if (*p)
					++p;
				else
					break;
			}
		}

		static void convertarg(const char *value, rate_t& ret)
		{
			int64_t numer = 0, denom = 0;
			const char *div = strchr(value, '/');
			if (div == nullptr) // no divisor sign
				denom = 1;
			else
				denom = atoll(div + 1);
			numer = atoll(value);
			ret = rate_t(numer, denom);
		}

	};
}