#pragma once

#define CPPLOG_FILTER_LEVEL LL_TRACE

#ifdef WIN32
#define __func__ __FUNCTION__
#endif

#define LOG_LEVEL(level, logger) CustomLogMessage(__FILE__, __func__, __LINE__, (level), logger).getStream()
#include "log/cpplog.hpp"

class CustomLogMessage : public cpplog::LogMessage
{
public:
	CustomLogMessage(const char* file, const char* function,
		unsigned int line, cpplog::loglevel_t logLevel,
		cpplog::BaseLogger &outputLogger)
		: cpplog::LogMessage(file, line, logLevel, outputLogger, false),
		m_function(function)
	{
		InitLogMessage();
	}

	static const char* shortLogLevelName(cpplog::loglevel_t logLevel)
	{
		switch( logLevel )
		{
		case LL_TRACE: return "T";
		case LL_DEBUG: return "D";
		case LL_INFO:  return "I";
		case LL_WARN:  return "W";
		case LL_ERROR: return "E";
		case LL_FATAL: return "F";
		default:       return "O";
		};
	}

protected:
	virtual void InitLogMessage()
	{
#ifdef _DEBUG
		m_logData->stream
			<< "[zm]["<<shortLogLevelName(m_logData->level)<<"|"
			<< m_logData->fileName<<":" 
			<< m_logData->line<< "]";

#else
		m_logData->stream
			<< "["
			<< m_logData->fileName << ":"
			<< m_logData->line
			<< "]["
			<< shortLogLevelName(m_logData->level)
			<< "]["
			<<m_logData->utcTime.tm_year+1900<<"-"
			<<m_logData->utcTime.tm_mon+1<<"-"
			<<m_logData->utcTime.tm_mday<<" "
			<<m_logData->utcTime.tm_hour<<":"
			<<m_logData->utcTime.tm_min<<":"
			<<m_logData->utcTime.tm_sec<<"] ";
#endif
	}
private:
	const char *m_function;
};

#ifdef _CONSOLE
__declspec(selectany) cpplog::StdErrLogger __g_c_log;
#else
__declspec(selectany) cpplog::OutputDebugStringLogger __g_c_log;
#endif

#ifdef _DEBUG
__declspec(selectany) cpplog::FileLogger __g_ff_log("zm_dbg.log");
#else
__declspec(selectany) cpplog::FileLogger __g_file_log("zm.log", true);
__declspec(selectany) cpplog::FilteringLogger __g_ff_log(LL_INFO, &__g_file_log);
#endif

//==================
//实际使用的全局log
__declspec(selectany) cpplog::MultiplexLogger glog(&__g_c_log, &__g_ff_log);

inline std::string __log_ws2s(const std::wstring& ws)
{
	std::string curLocale = setlocale(LC_ALL, NULL);        // curLocale = "C";
	setlocale(LC_ALL, "chs");
	const wchar_t* _Source = ws.c_str();
	size_t _Dsize = 2 * ws.size() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest,0,_Dsize);
	std::size_t c_size = 0;
	wcstombs_s(&c_size, _Dest, _Dsize, _Source,_Dsize);
	std::string result = _Dest;
	delete []_Dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

inline std::wstring __log_s2ws(const std::string& s)
{
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

//==============================================================
//自定义扩展类型
template<class _Elem,class _Traits>
std::basic_ostream<_Elem,_Traits>& operator<<(std::basic_ostream<_Elem,_Traits>& os, std::wstring wstr){
	std::string str = __log_ws2s(wstr);
	os<<str;
	return os;
}