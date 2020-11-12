#pragma once
#include <vector>
#include <exception>
#include <atomic>
#include "idx.h"
#include <iostream>

namespace sel {
	/*
	queue class designed and optimized for Windows overlapped I/O.
	Data can be read and written one element at a time, or in blocks.
	The special functions acquirewrite() and endwrite() are used for
	reading using overlapped I/O:

	Prior to calling an overlapped I/O read function (e.g. ReadFileEx()),
	Call acquirewrite() to indicate that the buffer is going to be written to
	asynchronously.

	It is up to the caller to ensure that no writes to the buffer are done
	until the overlapped read has completed.

	If a write is attempted while a write is pending, it will SILENTLY FAIL (i.e. it does
	not attempted to spin until the the async operation is completed.


	In the overlapped read completion routine, call endwrite(howmany) with the
	number of bytes actually received.
	endwrite() wraps any data received that has overflowed the logical buffer size (SZ),
	so that the buffer remains circular of size SZ, and the internal g and p
	pointers remain consistent.

	For overlapped writes, use the corresponding functions acquireread() and endread().

	GUARANTEED_SINGLE_THREADED should be set to true for single-threaded scenarios,
	which gives a slight performance improvement.


	*/

	template<bool GUARANTEED_SINGLE_THREADED>class quick_queue_lock {
	public:
		// NOTE: returns false if successful, true if already locked
		bool lock();
		void unlock();
	};

	template<> class quick_queue_lock<false> {
		std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
	public:
		bool lock() { return lock_.test_and_set(); }
		void unlock() { lock_.clear(); }

	};
// #define DEBUG_QUICK_QUEUE
	template<> class quick_queue_lock<true> {
#ifdef DEBUG_QUICK_QUEUE
		bool lock_ = false;
#endif
	public:
		bool lock() { 
#ifdef DEBUG_QUICK_QUEUE
			assert(!lock_);
			lock_ = true;
#endif
			return false; 
		}
		void unlock() {
#ifdef DEBUG_QUICK_QUEUE
			assert(lock_);
			lock_ = false;
#endif

		}
	};

	namespace  quick_queue_ut { class test; }

	template<typename T, const size_t SZ=dynamic_size_v, bool GUARANTEED_SINGLE_THREADED=true>class quick_queue 
		
	{
		friend class quick_queue_ut::test;

		std::vector<T> buf;
		//T *buf;		// Data is stored in a simple array, twice as big as we need
		idx<SZ> p;	// put() idx
		idx<SZ> g;	// get() idx
		size_t n;	// count of items in buf

		quick_queue_lock<GUARANTEED_SINGLE_THREADED>rLock;
		quick_queue_lock<GUARANTEED_SINGLE_THREADED>wLock;

	public:
		constexpr size_t size() const { return p.size(); }

		bool isempty() const { return n == 0; }
		bool isfull() const { return n == size(); }
		size_t get_avail() const { return n; }
		size_t put_avail() const { return size() - n; }

		quick_queue(size_t capacity=SZ) : p(capacity), g(capacity), n(0)
		{
//			std::cerr << "quick_queue ctor: SZ: " << SZ << ", capacity: " << capacity << std::endl;

			if (capacity > SZ)
				throw std::runtime_error("Quick queue capacity cannot be > SZ");
			else if (capacity == 0)
				throw std::runtime_error("Quick queue capacity cannot be 0");
			
			buf.resize(2 * size());

		}

		void put(const T& v)
		{

			if (isfull())
				throw std::runtime_error("put: buffer is full");

			if (!wLock.lock()) {
				++n;
				buf[p + size()] = v;
				buf[p++] = v;
				wLock.unlock();
			}

		}

		template<typename VectorLikeT> void atomicwrite(VectorLikeT& vec)
		{
			atomicwrite(vec.data(), vec.size());
		}


		void atomicwrite(const T* data, size_t howmany)
		{
			if (data && howmany) {
				if (put_avail() < howmany)
					throw std::runtime_error("atomicwrite: cannot write all requested data");

				if (!wLock.lock()) {
					while (howmany--) {
						++n;
						buf[p + size()] = *data;
						buf[p++] = *data++;

					}
					wLock.unlock();
				} else {
					throw std::runtime_error("atomicwrite: locked");
				}


			}
		}

		T &get()
		{
			if (n == 0)
				throw std::runtime_error("get: buffer is empty");

			if (rLock.lock())
				throw std::runtime_error("get: locked");

			--n;
			T &ret = buf[g++];
			rLock.unlock();
			return  ret;

		}

		const T &peek() const
		{
			if (n == 0)
				throw std::runtime_error("peek: buffer is empty");

			return  buf[g];
		}

		//void get( T* va, size_t howmany )
		//{ 
		//	if (get_avail() < howmany)
		//		throw std::runtime_error("get: cannot read all requested data");
		//	while (howmany--)
		//		*va++ = get();		
		//}

		T* acquirewrite() {
			return wLock.lock() ? 0 : &buf[p];
		}

		void endwrite(size_t howmany)
		{
			if (howmany) {
				if (put_avail() < howmany)
					throw std::runtime_error("endwrite: cannot write all requested data");
				n += howmany;

				// some data may overrun the buffer, find out how much
				auto pp = static_cast<size_t>(p) + howmany;
				auto overrun = (pp <= size()) ? 0 : pp - size()+1;
				// now copy the extra to the beginning of the buffer

				while (overrun--)
					buf[overrun] = buf[pp--];

				p += (int)howmany;

			}
			wLock.unlock();
		}

		const T* acquireread()
		{
			return rLock.lock() ? 0 : &buf[g];
		}

		const T* atomicread(size_t howmany)
		{
			auto ret = acquireread();
			if (ret)
				endread(howmany);
			return ret;
		}

		template<class It>void atomicread_into(It first, It last)
		{
			auto howmany = last - first;
			
			auto data = acquireread();
			if (data)
				endread(howmany);
			do {
				*first++ = *data++;
			} while (first != last);

		}

		template<typename Iterable>void atomicread_into(Iterable& dest)
		{
			atomicread_into(dest.begin(), dest.end());
		}

		void endread(const size_t howmany)
		{
			if (get_avail() < howmany)
				throw std::runtime_error("endread: cannot read all requested data");
			n -= howmany;
			// some data may have be requested past the physical end of the buffer.
			// find out how much
			size_t gg = (size_t)g + howmany;
			size_t overrun = (gg <= size()) ? 0 : gg - size()+1;
			g += (int)howmany;
			// now copy the beginning of the buffer to the extra
	
			while (overrun--)
				buf[overrun] = buf[gg--];

			rLock.unlock();
		}
	};

} // sel
#if defined(COMPILE_UNIT_TESTS)
#include "quick_queue_ut.h"
#endif