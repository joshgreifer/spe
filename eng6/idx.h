#pragma once
#include <climits>
#include <cassert>
namespace sel {
	/*
	An index class
	*/
	// index class wth SZ defined at compile time (i.e. in template parameter)

	constexpr size_t dynamic_size_v=std::numeric_limits<std::size_t>::max();

	template<size_t SZ>struct idx_traits {
		static constexpr bool is_power_of_2 = (SZ != 0 && (SZ & (SZ - 1)) == 0);
	};


	template<size_t SZ, bool = idx_traits<SZ>::is_power_of_2>class idx {
	};

	// non powers of two
	template<size_t SZ>class idx<SZ, false> {
		size_t i = 0;
		void make_modulo() { i %= SZ; }
	public:
        void reset() { i = 0; }
		constexpr size_t size() const { return SZ; }

		explicit idx(const size_t sz=SZ) : i(0) {}

		operator size_t() const { return i; }
		size_t operator++() { if (++i == SZ) i = 0; return i; }
		size_t operator--() { if (--i < 0) i += SZ; return i; }
        size_t operator+=(size_t n) { i += n; make_modulo(); return i; }
        size_t operator+=(idx n) { i += n; if (i >= SZ) i-=SZ; return i; } // guaranteed not to overflow
        size_t operator-=(size_t n) { i -= n; make_modulo(); return i; }
		size_t operator++(int) { size_t temp = i; if (++i == SZ) i = 0; return temp; }
		size_t operator--(int) { size_t temp = i; if (i == 0) i += SZ; --i; return temp; }
		//friend idx operator+(idx a, idx b) { return idx(a.i + b.i); }
		//friend idx operator-(idx a, idx b) { return idx(a.i - b.i); }
		//friend idx operator*(idx a, idx b) { return idx(a.i * b.i); }
		//friend idx operator/(idx a, idx b) { return idx(a.i / b.i); }

	};

	// powers of two
	template<size_t SZ>class idx<SZ, true> {
		inline void make_modulo() { i &= SZ - 1; }
		size_t i = 0;
	public:
        void reset() { i = 0; }
		constexpr size_t size() const { return SZ; }
		explicit constexpr idx(const size_t sz=SZ)  { }
		operator size_t() const { return i; }
		size_t operator++() { ++i; make_modulo();  return i; }
		size_t operator--() { --i; make_modulo(); return i; }
		size_t operator+=(size_t n) { i += n; make_modulo(); return i; }
		size_t operator-=(size_t n) { i -= n; make_modulo(); return i; }
		size_t operator++(int) { size_t temp = i; ++i; make_modulo(); return temp; }
		size_t operator--(int) { size_t temp = i; --i; make_modulo(); return temp; }
	};

	// unsigned short
	template<>class idx<USHRT_MAX+1, true> {
		void make_modulo() { } /* not needed  */
		unsigned short i;
		
	public:
        void reset() { i = 0; }
		constexpr size_t size() const { return USHRT_MAX+1; }
		explicit constexpr idx(const size_t sz=USHRT_MAX+1) : i(0)  {
			//		printf("ushort idx\n");
		}
		operator size_t() const { return i; }
		size_t operator++() { ++i; make_modulo();  return i; }
		size_t operator--() { --i; make_modulo(); return i; }
		size_t operator+=(size_t n) { i += (unsigned short)n; make_modulo(); return i; }
		size_t operator-=(size_t n) { i -= (unsigned short)n; make_modulo(); return i; }
		size_t operator++(int) { size_t temp = i; ++i; make_modulo(); return temp; }
		size_t operator--(int) { size_t temp = i; --i; make_modulo(); return temp; }
	};

	// unsigned char
	template<>class idx<UCHAR_MAX+1, true> {
		void make_modulo() { } /* not needed */
		unsigned char i;
	public:
        void reset() { i = 0; }
		constexpr size_t size() const { return UCHAR_MAX+1; }
		explicit constexpr idx(const size_t sz=UCHAR_MAX+1) : i(0) {
			//		printf("uchar idx\n");
		}
		operator size_t() const { return i; }
		size_t operator++() { ++i; make_modulo();  return i; }
		size_t operator--() { --i; make_modulo(); return i; }
		size_t operator+=(size_t n) { i += (unsigned char)n; make_modulo(); return i; }
		size_t operator-=(size_t n) { i -= (unsigned char)n; make_modulo(); return i; }
		size_t operator++(int) { size_t temp = i; ++i; make_modulo(); return temp; }
		size_t operator--(int) { size_t temp = i; --i; make_modulo(); return temp; }
	};

		// SZ==std::numeric_limits<std::size_t>::max(), actual sz determined in constructor
	template<>class idx<dynamic_size_v, false> {
		const size_t SZ;
		size_t i;
		void make_modulo() { i %= SZ; }
	public:
        void reset() { i = 0; }
		constexpr size_t size() const { return SZ; }
		explicit constexpr idx(const size_t sz) : SZ(sz), i(0) {
			assert(sz != dynamic_size_v);
		}
		operator size_t() const { return i; }
		size_t operator++() { if (++i == SZ) i = 0; return i; }
		size_t operator--() { if (--i < 0) i += SZ; return i; }
		size_t operator+=(size_t n) { i += n; make_modulo(); return i; }
		size_t operator-=(size_t n) { i -= n; make_modulo(); return i; }
		size_t operator++(int) { size_t temp = i; if (++i == SZ) i = 0; return temp; }
		size_t operator--(int) { size_t temp = i; if (i == 0) i += SZ; --i; return temp; }
		//friend idx operator+(idx a, idx b) { return idx(a.i + b.i); }
		//friend idx operator-(idx a, idx b) { return idx(a.i - b.i); }
		//friend idx operator*(idx a, idx b) { return idx(a.i * b.i); }
		//friend idx operator/(idx a, idx b) { return idx(a.i / b.i); }

	};

	using vidx=idx<dynamic_size_v, false>;


	template<class T, size_t SZ>class modulo_ptr
	{
		T * base_ ;
		mutable idx<SZ>offset_;
		T * ptr() const { return base_ + offset_;}
	public:
		constexpr size_t size() const { return offset_.size(); }

		explicit modulo_ptr(const size_t sz=SZ) {}

		auto& operator=(T *base) { base_ = base; return *this; }

		operator const T* () const { return ptr(); }
		operator T* () { return ptr(); }

		auto& operator++() { ++offset_; return *this; }
		auto& operator--() { --offset_; return *this; }
		auto& operator+=(size_t n) { offset_ += n; return *this; }
		auto& operator-=(size_t n) { offset_ -= n; return *this; }
		auto operator++(int) { auto temp(*this); ++offset_; return temp; }
		auto operator--(int) { auto temp(*this); --offset_; return temp; }
	};
} // sel