#pragma once
/**
<summary>
<p>Copyright (c) 1994-2007 Josh Greifer</p>
<p>Distributed under the <a href="http://www.opensource.org/licenses/mit-license.html">MIT Open Source Licence</a>, given in full below.</p>
<p/>
<p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:</p>
<p>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.</p>
<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</p>
</summary>
*/
#pragma once
#include <math.h>
#include <type_traits>

namespace sel {

	// GCD
	template<class M, class N> constexpr std::common_type_t<M, N> gcd(M a, N b)
	{
		return (b == 0) ? a : gcd(b, a % b);
	}
	// LCM
	template<class M, class N> constexpr std::common_type_t<M, N> lcm(M a, N b)
	{
		return a * b / gcd(a, b);
	}

	// CEIL(INTEGER_QUOTIENT)
	template<class M, class N> constexpr std::common_type_t<M, N> quotient_ceil(M num1, N num2)
	{
		std::common_type_t<M, N> ret = num1 / num2;
		if (num1 % num2 != 0)
			++ret;
		return ret;
	}

	template<unsigned int N>struct tPOW10
	{
		enum { _val = 10 * tPOW10<N - 1>::_val };
	};
	template<> struct tPOW10<0>
	{
		enum { _val = 1 };
	};

#ifdef POW10
#error redefinition of POW10
#endif

#define POW10(N) ((long long)(tPOW10<N>::_val))

	/* -----------------------------------------------------------------------------------------------
	Ratio class.
	*/
	template<typename INT_TYPE, typename UINT_TYPE, unsigned int MAX_DIGITS>class RATIO
	{
		using NUMER_TYPE = INT_TYPE;
		using DENOM_TYPE = UINT_TYPE;

		using INTMATH_EXCEPTION_DENOM_IS_ZERO = std::exception;
		using INTMATH_EXCEPTION_ACCURACY_LOST = std::exception;
		using INTMATH_EXCEPTION_OVERFLOW = std::exception;


		static const int MDOVER2 = MAX_DIGITS / 2;
		static const DENOM_TYPE max_denom = (DENOM_TYPE)POW10(MDOVER2) * POW10(MDOVER2);

		static const DENOM_TYPE DENOM_TYPE_HIGHBIT = (DENOM_TYPE)1 << (sizeof(INT_TYPE) * 8 - 1);
		static const NUMER_TYPE NUMER_TYPE_MAX = ~DENOM_TYPE_HIGHBIT;


		NUMER_TYPE		numer;
		DENOM_TYPE		denom;
		mutable	bool	dirty = true;

	public:
		// constructors
		RATIO() :
			numer(0),
			denom(1),
			dirty(false)
		{}

		RATIO(const RATIO &other)
		{
			*this = other;
		}


		RATIO(const NUMER_TYPE n, const DENOM_TYPE d = 1) :
			numer(n),
			denom(d)
		{
			if (d == 0)
				throw std::overflow_error("Divide by zero");

			dirty = (sel::gcd(numer, denom) != 1);
		}

		// Got this one off the net
		explicit RATIO(const double value)
		{
			const double epsilon = 1.0 / max_denom;
			const unsigned int maxIterations = 1000;

			double r0 = value;
			NUMER_TYPE a0 = (NUMER_TYPE)r0;

			// check for (almost) integer arguments, which should not go
			// to iterations.
			if (fabs(a0 - value) < epsilon) {
				numer = a0;
				denom = 1;
				return;
			}

			NUMER_TYPE p0 = 1;
			DENOM_TYPE q0 = 0;
			NUMER_TYPE p1 = a0;
			DENOM_TYPE q1 = 1;

			NUMER_TYPE p2 = 0;
			DENOM_TYPE q2 = 1;

			for (unsigned int n = 0; ; ++n) {

				double r1 = 1.0 / (r0 - a0);
				NUMER_TYPE a1 = (NUMER_TYPE)(r1);
				p2 = (a1 * p1) + p0;
				q2 = (a1 * q1) + q0;

				double convergent = (double)p2 / (double)q2;

				if (fabs(convergent - value) > epsilon) {
					p0 = p1;
					p1 = p2;
					q0 = q1;
					q1 = q2;
					a0 = a1;
					r0 = r1;
				}
				else {
					break;
				}

				if (n == maxIterations)
					throw std::underflow_error("cannot convert value to ratio");
			}


			numer = p2;
			denom = q2;
			dirty = true;


		}

		// reduce to lowest terms, i.e. divide by gcd(numer,denom)
		inline RATIO reduced()
		{
			RATIO result = *this;
			if (dirty) {
				NUMER_TYPE g = sel::gcd(numer, denom);
				if (g > 1) {
					result.numer /= g;
					result.denom /= g;
				}
				result.dirty = false;
			}
			return result;
		}

		// accessors
		inline const NUMER_TYPE n() const { return numer; }
		inline const DENOM_TYPE d() const { return denom; }

		// conversion operators
		operator double() const { return numer / (double)denom; }

		// syntactic sugar for reciprocal
		inline RATIO operator~() const
		{
			RATIO timer_(*this);
			timer_.recip();
			return timer_;
		}

		// reciprocal (swap numer, denom) - make sure sign bits are correct afterwards
		inline auto& recip()
		{
			if (denom & DENOM_TYPE_HIGHBIT)  // sign bit in denominator needs to be clear
				throw INTMATH_EXCEPTION_OVERFLOW();
			NUMER_TYPE temp = (NUMER_TYPE)denom;
			if (numer < 0) {
				temp = -temp;
				denom = (DENOM_TYPE)-numer;
			}
			else {
				denom = (DENOM_TYPE)numer;
			}
			numer = temp;
			return *this;
		}
		// equality
		inline bool operator==(const RATIO& other) const
		{
			return this->numer * other.denom == this->denom * other.numer;
		}

		inline bool operator!=(const RATIO& other) const
		{
			return this->numer * other.denom != this->denom * other.numer;
		}

		RATIO& operator+=(int i) { numer += i * denom; dirty = true; return *this; }
		RATIO& operator++() { ++numer; dirty = true; return *this; }
		RATIO operator++(int) const { RATIO temp = this; ++*this; return temp; }
		RATIO& operator-=(int i) { numer -= i * denom; dirty = true; return *this; }
		RATIO& operator--() { --numer;  dirty = true; return *this; }
		RATIO operator--(int) { RATIO temp = this; --*this; return temp; }

		RATIO& operator*=(int i) { numer *= i; dirty = true; return *this; }

		RATIO& operator/=(int i) {
			bool neg = (i < 0);
			if (neg)
				i = -i;
			denom *= i;
			if (neg)
				numer = -numer;
			dirty = true;
			return *this;
		}

		RATIO& operator+=(const RATIO& other) {
			dirty = true;
			numer = numer * other.denom + denom * other.numer;
			denom = denom * other.denom;
			if (denom & DENOM_TYPE_HIGHBIT)  // sign bit in denominator needs to be clear
				throw INTMATH_EXCEPTION_OVERFLOW();
			return *this;
		}
		RATIO& operator-=(const RATIO& other) {
			dirty = true;
			numer = numer * other.denom - denom * other.numer;
			denom = denom * other.denom;
			if (denom & DENOM_TYPE_HIGHBIT)  // sign bit in denominator needs to be clear
				throw INTMATH_EXCEPTION_OVERFLOW();
			return *this;
		}
		RATIO& operator*=(const RATIO& other) {
			dirty = true;
			numer = numer * other.numer;
			denom = denom * other.denom;
			if (denom & DENOM_TYPE_HIGHBIT)  // sign bit in denominator needs to be clear
				throw INTMATH_EXCEPTION_OVERFLOW();
			return *this;
		}
		RATIO& operator/=(const RATIO& other) {
			dirty = true;
			numer = numer * other.denom;
			denom = denom * other.numer;
			if (denom & DENOM_TYPE_HIGHBIT)  // sign bit in denominator needs to be clear
				throw INTMATH_EXCEPTION_OVERFLOW();
			return *this;
		}

		RATIO operator+(int i) const { RATIO sum = *this; return sum += i; }
		RATIO operator-(int i) const { RATIO sum = *this; return sum -= i; }
		RATIO operator*(int i) const { RATIO sum = *this; return sum *= i; }
		RATIO operator/(int i) const { RATIO sum = *this; return sum /= i; }

		RATIO operator+(const RATIO& other) const { RATIO sum = *this; return sum += other; }
		RATIO operator-(const RATIO& other) const { RATIO sum = *this; return sum -= other; }
		RATIO operator*(const RATIO& other) const { RATIO sum = *this; return sum *= other; }
		RATIO operator/(const RATIO& other) const { RATIO sum = *this; return sum /= other; }


		// level:  make denominators equal
		static inline void level(RATIO& r1, RATIO& r2)
		{
			r1 = r1.reduced();
			r2 = r2.reduced();

			NUMER_TYPE g = sel::gcd(r1.denom, r2.denom);
			NUMER_TYPE m1 = r2.denom / g;
			NUMER_TYPE m2 = r1.denom / g;
			if (m1 > 1) {
				r1.numer *= m1;
				r1.denom *= m1;
			}
			if (m2 > 1) {
				r2.numer *= m2;
				r2.denom *= m2;
			}
		}
	};

#include <iostream>

	template<typename INT_TYPE, typename UINT_TYPE, unsigned int MAX_DIGITS> std::ostream& operator<<(std::ostream& os, RATIO<INT_TYPE, UINT_TYPE, MAX_DIGITS>& r)
	{
		os << r.n() << '/' << r.d();
		return os;
	}

	//default numdigits is 8
	typedef RATIO<int, unsigned int, 8> Ratio8;
	typedef RATIO<int64_t, uint64_t, 16> Ratio;



	//#pragma message( "( included " __FILE__ ")" )
} // sel