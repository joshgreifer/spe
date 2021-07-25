#pragma once
#include <map>

#ifdef __GNUC__
#define _stricmp strcasecmp
#endif
// comparators for map<const char *, T>
namespace sel {
	struct  cstring_less {
		bool operator() (const std::string &p, const std::string &q) const { return strcmp(p.c_str(), q.c_str()) < 0; }
	};
	// case-insensitive
	struct  cstring_lessi {
		bool operator()(const std::string &p, const std::string &q) const
		{
			return _stricmp(p.c_str(), q.c_str()) < 0;
		}
	};


	template<typename ITEM_T>struct case_insensitive_dictionary : public std::map < std::string, ITEM_T, cstring_lessi>
	{
		const char *key_for(const ITEM_T& val) {
			for (auto& kv : *this)
				if (kv.second == val)
					return kv.first.c_str();
			return nullptr;


		}
	};
	template<typename ITEM_T>struct case_sensitive_dictionary : public std::map < std::string, ITEM_T, cstring_less>
	{
		const char *key_for(const ITEM_T& val) {
			for (auto& kv : *this)
				if (kv.second == val)
					return kv.first.c_str();
			return nullptr;


		}

	};

} // sel
