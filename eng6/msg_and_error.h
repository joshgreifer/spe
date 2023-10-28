#pragma once
#include <stdio.h>
#include <ctime>
// Make message
#include "params.h"
#include <array>
#include <stdexcept>
#include <stdarg.h>
#include <string>
#include <system_error>
#include "singleton.h"

namespace sel  {

template <size_t SZ = 65536> class charbuf : public std::array<char, SZ> {


public:
	charbuf() {}
	charbuf(const char *s) { write(s); }
	charbuf(char *s) { write(s); }
	static constexpr size_t SIZE = SZ;

	auto& strftime(const char *fmt, std::time_t timer_)
	{
		std::strftime((char *)data(), SZ, fmt, reinterpret_cast<const tm *>(timer_));
		return *this;
	}

	auto& sprintf(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		vsnprintf((char *)data(), SZ, fmt, args);
		return *this;
	}

	auto& vsprintf(const char *fmt, va_list args)
	{
		vsnprintf((char *)this->data(), SZ, fmt, args);
		return *this;

	}

	auto& write(const char *s)
	{
		strcpy((char *)data(), s);
		return *this;
	}

	auto& append(const char *s)
	{
		strcat((char *)data(), s);
		return *this;

	}

	auto& loadFromTextFile(const char *filename)
	{
		FILE *fp = fopen(filename, "r");
		if (fp) {
			size_t len = fread((void *)data(), sizeof(char), SZ - 1, fp);
			fclose(fp);
			((char *)data())[len++] = '\0';
		}
		else
			throw std::runtime_error("Couldn't open file");
		return *this;


	}

	auto& macro_expand(params& macros)
	{
		// Perform substitution in-place

		const char *ARG_L_DELIM = "${";
		const char *ARG_R_DELIM = "}";

		const char *srcp = data();
		charbuf doc_;
		char *dstp = (char *)doc_.data();

		char *next_arg_l_delim;
		char *next_arg_r_delim;
		const char *macro_name;

		while (next_arg_l_delim = (char  *)strstr(srcp, ARG_L_DELIM)) {
			if (!(next_arg_r_delim = strstr(next_arg_l_delim, ARG_R_DELIM))) {
				this->sprintf("No closing %s at %20.20s", ARG_R_DELIM, next_arg_l_delim);
				throw std::runtime_error(*this);
			}
			*next_arg_l_delim = '\0'; // for strcat
			*next_arg_r_delim = '\0'; // make macro_name into c string
			macro_name = next_arg_l_delim + 2; // strlen(ARG_L_DELIM)
			while (*dstp = *srcp++)
				dstp++;

			// look for macro value
			const char *macro_value = nullptr;
			for (auto& kv : macros) {
				if (!strcmp(kv.first.c_str(), macro_name)) {
					macro_value = kv.second;
					break;
				}
			}
			if (!macro_value) {
				this->sprintf("Macro %s not found", macro_name);
				throw std::runtime_error(*this);

			}
			while (*dstp = *macro_value++)
				dstp++;
			srcp = next_arg_r_delim + 1;

		}

		while (*dstp++ = *srcp++);
		write(doc_);
		return *this;

	}

	std::string as_string() const { return this->data(); }
	const char *data() const { return std::array<char, SZ>::data(); }
	operator const char*() { return std::array<char, SZ>::data(); }
//	operator const std::string() { return std::string(this->data()); }

	auto& operator=(const char* s) { return write(s);  }

	virtual ~charbuf() {}

};

typedef charbuf<65536U> charbuf_long;
typedef charbuf<2048U> charbuf_short;

template<size_t SZ=2048U> std::string format_message(const char *fmt, ...)
{
	charbuf<SZ> buf;

	va_list args;
	va_start(args, fmt);
	buf.vsprintf(fmt, args);
	return buf.as_string();
}

template<size_t SZ> std::string os_err_message(int os_errno)
{
	const char *os_errmsg = std::system_error(os_errno, std::system_category()).what();
	return 	os_errmsg;
	
}
typedef charbuf<65536U> charbuf_long;
typedef charbuf<2048U> charbuf_short;


struct eng_ex : public std::runtime_error {

	eng_ex(const std::string& msg) : runtime_error(msg) { }
//	eng_ex(charbuf_short& msg) : runtime_error(msg) { }
	eng_ex(const char *msg) : runtime_error(msg) { }
//	eng_ex(int os_errno) : runtime_error(os_err_message<2048U>(os_errno)) {}
	eng_ex(int os_errno) : runtime_error(strerror(os_errno)) {}

};
struct sys_ex : public std::runtime_error
{
	sys_ex(int os_errno = errno) : std::runtime_error(strerror(os_errno)) {}
};



}

// See https://akrzemi1.wordpress.com/examples/error_code-example/
// std::error_code implementation
# include <system_error>


enum class eng_errc
{
	// no 0
	input_stream_eof = 10, // End of file
};

//std::error_code make_error_code(eng_errc);

namespace { // anonymous namespace

	struct eng_error_category : std::error_category
	{
		const char* name() const noexcept override;
		std::string message(int ev) const override;
	};

	const char* eng_error_category::name() const noexcept
	{
		return "SEL SigProc Engine";
	}

	std::string eng_error_category::message(int ev) const
	{
		switch (static_cast<eng_errc>(ev))
		{
		case eng_errc::input_stream_eof:
			return "end of file on input stream";

		default:
			return "(unrecognized engine error)";
		}
	}

	const eng_error_category eng_error_category_{};

} // anonymous namespace


std::error_code make_error_code(eng_errc e)
{
	return { static_cast<int>(e),  eng_error_category_ };
}

namespace std
{
	template <>
	struct is_error_code_enum<eng_errc> : true_type {};
}
