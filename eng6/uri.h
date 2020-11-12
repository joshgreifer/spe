#pragma once
#include "msg_and_error.h"
#include "singleton.h"
// TODO: Add uri parsing
namespace sel {
	class uri {
	public:
		enum scheme
		{
			FILE,		// file:
			HTTP,		// http:
			HTTPS,		// https:
			BLUETOOTH,  // bt:
			USB,		// usb:
			PIPE,		// pipe:
			UDP,		// udp:
			TCP,		// tcp:
			COMM,		// comm:
			WS,			// ws: (WebSocket)
			AUDIO,		// audio: (audio mixer/subsystem)
			UNK
		};
	private:
		const char *_path;
		scheme _scheme;

	public:

		uri() = delete;
		uri(char* s) = delete;

		uri(const std::string& s) : uri(s.c_str()) {}

		uri(const char *cstr)
		{
			std::string s(cstr);
			size_t delim_pos = s.find(":");
			if (delim_pos == std::string::npos) {
				// No scheme, assume FILE
				_scheme = FILE;
				_path = cstr;
			}
			else {
				std::string scheme = s.substr(0, delim_pos);
				// check for two leading forward slashes
				delim_pos++;
				if (cstr[delim_pos++] != '/')
					throw eng_ex(format_message("Couldn't parse  uri '%s', missing forward slash", cstr, scheme.c_str()));
				if (cstr[delim_pos++] != '/')
					throw eng_ex(format_message("Couldn't parse  uri '%s', missing forward slash", cstr, scheme.c_str()));

				_path = cstr + delim_pos;

				try {
					_scheme = scheme_names::get().lookup(scheme);
				}
				catch (std::out_of_range&) {
					throw eng_ex(format_message("Couldn't parse  uri '%s', unknown URI scheme '%s'", cstr, scheme.c_str()));

					// _scheme = UNK;
				}
			}
		}


	private:

		struct scheme_names : public singleton<scheme_names>
		{
			case_sensitive_dictionary<scheme> _dict;

			scheme lookup(const std::string& k) { return _dict.at(k); }
			const char * reverselookup(scheme v) { return _dict.key_for(v); }

			scheme_names() {
				auto &dict = _dict;
				dict["http"] = HTTP;
				dict["https"] = HTTPS;
				dict["file"] = FILE;
				dict["tcp"] = TCP;
				dict["ws"] = WS;
				dict["audio"] = AUDIO;
				dict["(unknown)"] = UNK;
			}
		} scheme_names;

	public:
		const scheme scheme() const
		{
			return _scheme;
		}

		static const char *scheme_name(enum scheme s)
		{
			return scheme_names::get().reverselookup(s);
		}
		void assert_scheme(enum scheme s) const
		{
			if (scheme() != s) {
				throw std::runtime_error(format_message<100>("uri scheme is '%s', should be '%s'", scheme_name(scheme()), scheme_name(s)));

			}
		}
		const char *path() const { return _path; }


	};
}