#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include "StringFunc.h"


namespace zm{
	struct INIValue{
		std::string value_;
		INIValue(){};
		INIValue(const std::string& value){this->value_ = value;};

#ifndef _MSC_VER
		operator unsigned long long(){return atoll(value_.c_str());}
		operator long long(){return atoll(value_.c_str());}
#else
		operator unsigned long long(){return _atoi64(value_.c_str());}
		operator long long(){return _atoi64(value_.c_str());}
#endif


		operator unsigned long(){return atol(value_.c_str());}
		operator long(){return atol(value_.c_str());}
		operator unsigned short(){return atoi(value_.c_str());}
		operator short(){return atoi(value_.c_str());}
		operator unsigned char(){return atoi(value_.c_str());}
		operator unsigned int(){return atoi(value_.c_str());}
		operator int(){return atoi(value_.c_str());}
		operator double(){return atof(value_.c_str());}
		operator float(){return (float)atof(value_.c_str());}
		operator const char *(){return value_.c_str();}
		operator const std::string& (){return value_;}
	};


	struct INIFile{
		std::map<std::string, INIValue> values_;
		bool		isok_;
		INIFile(){isok_=false;};
		INIFile(const std::string& name){load(name);};
		bool is_ok(){
			return isok_;
		}
		bool load(const std::string& name){
			using namespace std;
			isok_=false;
			ifstream fi(name.c_str());
			if (!fi.good()) return false;

			std::auto_ptr<char> rbuf(new char[1024*64]);
			char *temp=rbuf.get();
			while(fi.getline(temp,1024*64)){
				string line=temp;
				boost::algorithm::trim(line);
				if (line.size()==0 || line[0]=='#') continue;

				std::vector<std::string> vs;
				boost::algorithm::split(vs,line,boost::algorithm::is_any_of("="));			

				if (vs.size()!=2 ) continue;

				boost::algorithm::trim(vs[0]);
				boost::algorithm::trim(vs[1]);


				values_[vs[0]]=vs[1];
			}
			isok_=true;
			return true;
		}


		bool parse(const string& contents){
			values_.clear();
			std::vector<std::string> vs;
			boost::algorithm::split(vs,contents,boost::algorithm::is_any_of("\r\n"));	


			for (size_t i = 0;i<vs.size();i++){
				string &line=vs[i];
				boost::algorithm::trim(line);
				if (line.size()==0 || line[0]=='#') continue;

				std::vector<std::string> vs;
				boost::algorithm::split(vs,line,boost::algorithm::is_any_of("="));			

				if (vs.size()!=2 ) continue;

				boost::algorithm::trim(vs[0]);
				boost::algorithm::trim(vs[1]);


				values_[vs[0]]=vs[1];
			}

			isok_=true;
			return true;
		}

		INIValue& operator [](const std::string &name){
			return values_[name];
		}


		INIValue& value(const std::string &name){
			return values_[name];
		}

		template<class T>
		INIValue value(const std::string &name,const T& default_value){
			std::map<std::string, INIValue>::iterator it = values_.find(name);
			if (it==values_.end()){
				return str_cast(default_value);
			}
			return it->second;
		}
	};

}
