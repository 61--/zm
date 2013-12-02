#pragma once
#include <string>
#include <boost/lexical_cast.hpp>

namespace zm{

	typedef std::basic_string<TCHAR> tstring;

	template<typename T>
	inline std::wstring to_wstring(T t){
		return boost::lexical_cast<std::wstring, T>(t);
	}

	inline std::wstring to_wstring(const std::string& s){
		setlocale(LC_ALL, "chs");
		const char* _Source = s.c_str();
		size_t _Dsize = s.size() + 1;
		wchar_t *_Dest = new wchar_t[_Dsize];
		wmemset(_Dest, 0, _Dsize);
		std::size_t c_size = 0;
		mbstowcs_s(&c_size, _Dest, _Dsize, _Source, _Dsize);
		std::wstring result = _Dest;
		delete[] _Dest;
		setlocale(LC_ALL, "C");
		return result;
	}

	inline std::wstring to_wstring(const std::wstring& ws){
		return ws;
	}
	
	template<typename T>
	inline std::string to_string(T t){
		return boost::lexical_cast<std::string, T>(t);
	}
	

	inline std::string to_string(const std::wstring& ws){
		std::string curLocale = setlocale(LC_ALL, NULL);        // curLocale = "C";
		setlocale(LC_ALL, "chs");
		const wchar_t* _Source = ws.c_str();
		size_t _Dsize = 2 * ws.size() + 1;
		char *_Dest = new char[_Dsize];
		memset(_Dest, 0, _Dsize);
		std::size_t c_size = 0;
		wcstombs_s(&c_size, _Dest, _Dsize, _Source, _Dsize);
		std::string result = _Dest;
		delete[]_Dest;
		setlocale(LC_ALL, curLocale.c_str());
		return result;
	}

	inline std::string to_string(const wchar_t* ws){
		return to_string(std::wstring(ws));
	}

	inline std::string to_string(wchar_t* ws){
		return to_string(std::wstring(ws));
	}

	inline std::string to_string(const std::string& s){
		return s;
	}

#ifdef UNICODE
	#define to_tstring to_wstring
#else
	#define to_tstring to_string
#endif



}//namespace