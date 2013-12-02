//==============================
//��־�������outputdebug_buf�᲻ͣ�ĵ������

#pragma once
#ifndef _OUTPUT_DEBUG_STREAM_H
#define _OUTPUT_DEBUG_STREAM_H

#include <windows.h>

template<typename TChar, typename TTraits = std::char_traits<TChar>>
class OutputDebugStringBuf : public std::basic_stringbuf<TChar,TTraits> {
public:
	explicit OutputDebugStringBuf() : _buffer(256) {
		setg(nullptr, nullptr, nullptr);
		setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
	}

	~OutputDebugStringBuf() {
	}

	static_assert(std::is_same<TChar,char>::value || std::is_same<TChar,wchar_t>::value, "OutputDebugStringBuf only supports char and wchar_t types");

	int sync() try {
		MessageOutputer<TChar,TTraits>()(pbase(), pptr());
		setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
		return 0;
	} catch(...) {
		return -1;
	}

	int_type overflow(int_type c = TTraits::eof()) {
		auto syncRet = sync();
		if (c != TTraits::eof()) {
			_buffer[0] = c;
			setp(_buffer.data(), _buffer.data() + 1, _buffer.data() + _buffer.size());
		}
		return syncRet == -1 ? TTraits::eof() : 0;
	}


private:
	std::vector<TChar>		_buffer;

	template<typename TChar, typename TTraits>
	struct MessageOutputer;

	template<>
	struct MessageOutputer<char,std::char_traits<char>> {
		template<typename TIterator>
		void operator()(TIterator begin, TIterator end) const {
			std::string s(begin, end);
			OutputDebugStringA(s.c_str());
		}
	};

	template<>
	struct MessageOutputer<wchar_t,std::char_traits<wchar_t>> {
		template<typename TIterator>
		void operator()(TIterator begin, TIterator end) const {
			std::wstring s(begin, end);
			OutputDebugStringW(s.c_str());
		}
	};
};


template <class _Elem>
class outputdebug_buf: public OutputDebugStringBuf<_Elem>{};


template <class _Elem>
class outputdebug_stream: public std::basic_ostream<_Elem>
{
public:
    outputdebug_stream() : std::basic_ostream<_Elem>(new outputdebug_buf<_Elem>())
    {}

    ~outputdebug_stream(){ delete this->rdbuf(); }
};

typedef outputdebug_stream<char> dbgwin_stream;
typedef outputdebug_stream<wchar_t> wdbgwin_stream;

#endif
