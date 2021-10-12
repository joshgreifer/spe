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

	template<class N>N constexpr pow10( N n)
	{
		return n == 0 ? 1 : 10 * pow10(n - 1);
	}
	
	/* -----------------------------------------------------------------------------------------------
	Ratio class.
	*/
	template<typename UINT_TYPE, unsigned int MAX_DIGITS=16>class Ratio
	{



		//using INTMATH_EXCEPTION_DENOM_IS_ZERO = std::exception;
		//using INTMATH_EXCEPTION_ACCURACY_LOST = std::exception;
		//using INTMATH_EXCEPTION_OVERFLOW = std::exception;


		static const int MDOVER2 = MAX_DIGITS / 2;
		static const UINT_TYPE max_denom = (UINT_TYPE)pow10(MDOVER2) * pow10(MDOVER2);


		UINT_TYPE		numer;
		UINT_TYPE		denom;
		mutable	bool	dirty = true;

	public:
		// constructors
		Ratio() :
			numer(0),
			denom(1),
			dirty(false)
		{}

		Ratio(const Ratio &other)
		{
			*this = other;
		}

        Ratio ( Ratio && ) = default;

        Ratio& operator=(const Ratio& other) {
            this->numer = other.numer;
            this->denom = other.denom;
            return *this;
        }

		Ratio(const UINT_TYPE n, const UINT_TYPE d = 1) :
			numer(n),
			denom(d)
		{
			if (d == 0)
				throw std::overflow_error("Divide by zero");

			dirty = (sel::gcd(numer, denom) != 1);
		}

		// Got this one off the net
		explicit Ratio(const double value)
		{
			const double epsilon = 1.0 / max_denom;
			const unsigned int maxIterations = 1000;

			double r0 = value;
			UINT_TYPE a0 = (UINT_TYPE)r0;

			// check for (almost) integer arguments, which should not go
			// to iterations.
			if (fabs(a0 - value) < epsilon) {
				numer = a0;
				denom = 1;
				return;
			}

			UINT_TYPE p0 = 1;
			UINT_TYPE q0 = 0;
			UINT_TYPE p1 = a0;
			UINT_TYPE q1 = 1;

			UINT_TYPE p2 = 0;
			UINT_TYPE q2 = 1;

			for (unsigned int n = 0; ; ++n) {

				double r1 = 1.0 / (r0 - a0);
				UINT_TYPE a1 = (UINT_TYPE)(r1);
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
		inline Ratio reduced()
		{
			Ratio result = *this;
			if (dirty) {
				UINT_TYPE g = sel::gcd(numer, denom);
				if (g > 1) {
					result.numer /= g;
					result.denom /= g;
				}
				result.dirty = false;
			}
			return result;
		}

		// accessors
		inline  auto n() const { return numer; }
		inline  auto d() const { return denom; }

		// conversion operators
		operator double() const { return numer / (double)denom; }

		// syntactic sugar for reciprocal
		inline Ratio operator~() const
		{
			Ratio timer_(*this);
			timer_.recip();
			return timer_;
		}

		// reciprocal (swap numer, denom) - make sure sign bits are correct afterwards
		inline auto& recip()
		{

			auto temp = denom;
	
			denom = numer;
			numer = temp;
			return *this;
		}
		// equality
		bool operator==(const Ratio& other) const
		{
			return this->numer * other.denom == this->denom * other.numer;
		}

        template<class T,  std::enable_if_t<std::is_integral_v<T>, int> = 0> bool operator==(const T other) const
        {
            return this->numer == other &&  this->denom == 1;
        }

		bool operator!=(const Ratio& other) const
		{
			return !operator==(other);
		}

		Ratio& operator+=(UINT_TYPE i) { numer += i * denom; dirty = true; return *this; }
		Ratio& operator++() { ++numer; dirty = true; return *this; }
		Ratio operator++(int) const { Ratio temp = this; ++*this; return temp; }
		Ratio& operator-=(UINT_TYPE i) { numer -= i * denom; dirty = true; return *this; }
		Ratio& operator--() { --numer;  dirty = true; return *this; }
		Ratio operator--(int) { Ratio temp = this; --*this; return temp; }

		Ratio& operator*=(UINT_TYPE i) { numer *= i; dirty = true; return *this; }

		Ratio& operator/=(UINT_TYPE i) {
			denom *= i;
			dirty = true;
			return *this;
		}

		Ratio& operator+=(const Ratio& other) {
			dirty = true;
			numer = numer * other.denom + denom * other.numer;
			denom = denom * other.denom;

			return *this;
		}
		Ratio& operator-=(const Ratio& other) {
			dirty = true;
			numer = numer * other.denom - denom * other.numer;
			denom = denom * other.denom;

			return *this;
		}
		Ratio& operator*=(const Ratio& other) {
			dirty = true;
			numer = numer * other.numer;
			denom = denom * other.denom;

			return *this;
		}
		Ratio& operator/=(const Ratio& other) {
			dirty = true;
			numer = numer * other.denom;
			denom = denom * other.numer;

			return *this;
		}

		Ratio operator+(UINT_TYPE i) const { Ratio sum = *this; return sum += i; }
		Ratio operator-(UINT_TYPE i) const { Ratio sum = *this; return sum -= i; }
		Ratio operator*(UINT_TYPE i) const { Ratio sum = *this; return sum *= i; }
		Ratio operator/(UINT_TYPE i) const { Ratio sum = *this; return sum /= i; }

		Ratio operator+(const Ratio& other) const { Ratio sum = *this; return sum += other; }
		Ratio operator-(const Ratio& other) const { Ratio sum = *this; return sum -= other; }
		Ratio operator*(const Ratio& other) const { Ratio sum = *this; return sum *= other; }
		Ratio operator/(const Ratio& other) const { Ratio sum = *this; return sum /= other; }


		// level:  make denominators equal
		static inline void level(Ratio& r1, Ratio& r2)
		{
			r1 = r1.reduced();
			r2 = r2.reduced();

			UINT_TYPE g = sel::gcd(r1.denom, r2.denom);
			UINT_TYPE m1 = r2.denom / g;
			UINT_TYPE m2 = r1.denom / g;
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

	template<typename UINT_TYPE, unsigned int MAX_DIGITS> std::ostream& operator<<(std::ostream& os, Ratio<UINT_TYPE, MAX_DIGITS>& r)
	{
		os << r.n() << '/' << r.d();
		return os;
	}

	//default numdigits is 8
	typedef Ratio<uint32_t, 8> Ratio32;
	typedef Ratio<uint64_t, 20> Ratio64;




	//#pragma message( "( included " __FILE__ ")" )
} // sel

#if defined(COMPILE_UNIT_TESTS)
#include "unit_test.h"

SEL_UNIT_TEST(ratio)

void run() {
    SEL_UNIT_TEST_ITEM("gcd");
    constexpr unsigned long long mersenne_prime1 = 2305843009213693951;
    for (size_t n = 2; n < 100000; ++n)
        SEL_UNIT_TEST_ASSERT(sel::gcd(mersenne_prime1, n) == 1);
}
SEL_UNIT_TEST_END
#endif