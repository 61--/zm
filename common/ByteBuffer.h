#pragma once

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include <vector>
#include <list>
#include <map>
#include <string>
#include <set>
#include <boost/smart_ptr.hpp>
using namespace std;


struct ByteBufferException
{
    ByteBufferException(const char *act, unsigned int rp, unsigned int wp, unsigned int rs, unsigned int cs=0)
    {
        action = act;
        rpos = rp;
        wpos = wp;
        readsize = rs;
        cursize = cs;
    }
    unsigned int rpos, wpos, readsize, cursize;
    const char *action;
};


#define BUF_VEC_WIDTH  2

#define BYTEBUFFER_OP(type)		ByteBuffer &operator<<(type value)\
{\
	append<type>(value);	\
	return *this;		\
}	\
ByteBuffer &operator>>(type &value) \
{\
	read<type>(value);	\
	return *this;\
}

//COW模式，多线程可能会有语义上的问题。
struct ByteBuffer
{
    public:
		//
		ByteBuffer(const void *data,unsigned int size,bool bcopy = false){
			rpos_=0;
			array_with_bytes_=BUF_VEC_WIDTH;
			if (bcopy){
				raw_data_ptr_=0;
				data_size_ = 0;
				storage_size_ = 0;
				append(data,size);
			}else{
				data_ptr_.reset();
				raw_data_ptr_=const_cast<unsigned char *>((unsigned char *)data);
				data_size_ = 0;
				storage_size_ = size;
			}
		}
		
		ByteBuffer(boost::shared_ptr<unsigned char> data,unsigned int size){
			array_with_bytes_=BUF_VEC_WIDTH;
			rpos_=0;
			data_size_=size;
			data_ptr_ = data;
			raw_data_ptr_=data.get();
			storage_size_ = data_size_;
		}

		ByteBuffer &operator=(const ByteBuffer& b)
		{
			array_with_bytes_=b.array_with_bytes_;
			rpos_ = 0;

			if (b.data_ptr_!=NULL){				//COW
				data_ptr_ = b.data_ptr_;
				data_size_ = b.data_size_;
				raw_data_ptr_=b.raw_data_ptr_;
				storage_size_ = data_size_;

			}else{
				raw_data_ptr_=0;
				data_size_ = 0;
				storage_size_ = 0;
				append(b.contents(),b.size());
			}
			return *this;
		}

		ByteBuffer(const ByteBuffer& b){
			*this = b;
		}
		
		ByteBuffer(){
			rpos_=0;
			wpos_=0;
			storage_size_ = 0;
			raw_data_ptr_ = 0;
			array_with_bytes_=BUF_VEC_WIDTH;
		}
		
        unsigned char operator[](size_t pos)
        {
            return raw_data_ptr_[pos];
        }
		
        size_t rpos()
        {
            return rpos_;
        };

        size_t rpos(size_t rpos)
        {
            rpos_ = rpos < size() ? rpos :size();
            return rpos_;
        };

        size_t wpos()
        {
            return wpos_;
        }

        size_t wpos(size_t wpos)
        {
            wpos_ = wpos < storage_size_ ? wpos : storage_size_;
            return wpos_;
        }
				
        template <typename T> T read()
        {
            T r;
            read(r);
            return r;
        };

		template <typename T> void read(T& value)
		{
			read(value,rpos());
			rpos_ += sizeof(T);
		};
		
        unsigned char *contents() const { return (unsigned char *)dataptr(); };
		
		void peek(size_t pos,unsigned char *dest, size_t len)
		{
			if (pos + len <= size())
			{
				memcpy(dest, (dataptr()+pos), len);
			}
			else
			{
				throw ByteBufferException("read-into", pos, wpos_, len,data_size_);
			}
		}

		void get(unsigned char *dest, size_t len)
		{
			peek(rpos_,dest,len);
			rpos_ += len;
		}
		
        void append(const void *src, size_t cnt)
        {
            if (!cnt) return;
			if (storage_size_< wpos_ + cnt){
				if (!selfmemory())
					throw ByteBufferException("out of memeory range", rpos_, wpos_, data_size_);
				
				resize( ((wpos_ + cnt+ 1024 )/4096 + 1) * 4096);
			}

			//COW,写的时候，数据指针必然只有一个copy，如果有大于1个，那么新复制一份
			if (data_ptr_.use_count()>1){
				resize(storage_size_);
			}
						
            memcpy(dataptr()+wpos_, src, cnt);
            wpos_ += cnt;
        }

		
        void put(size_t pos, const void *src, size_t cnt)
        {
            memcpy(dataptr()+pos, src, cnt);
        }
		
		//当使用外部内存时，storage_size_是数据大小
		unsigned int size() const{
			if (selfmemory()){
				return data_size_;
			}
			return storage_size_;
		};

		void resize(unsigned int size){
			if (!selfmemory())
				throw ByteBufferException("can not resize", rpos_, wpos_, data_size_);
			boost::shared_ptr<unsigned char> p(new unsigned char[size]);
			unsigned int copysize = size>=this->size()?this->size():size;
			if (this->size()){
				memcpy(p.get(),dataptr(),copysize);
			}
			data_ptr_ = p;
			raw_data_ptr_ = data_ptr_.get();
			storage_size_ = size;
		}
		
		BYTEBUFFER_OP(char);
		BYTEBUFFER_OP(unsigned char);
		BYTEBUFFER_OP(short);
		BYTEBUFFER_OP(unsigned short);
		BYTEBUFFER_OP(int);
		BYTEBUFFER_OP(unsigned int);
		BYTEBUFFER_OP(long);
		BYTEBUFFER_OP(unsigned long);
		BYTEBUFFER_OP(float);
		BYTEBUFFER_OP(double);
		BYTEBUFFER_OP(long long);
		BYTEBUFFER_OP(unsigned long long);

		ByteBuffer &operator<<(bool value)
		{
			append<unsigned char>(value);
			return *this;
		}

		
		template<typename T,size_t N>
		ByteBuffer  &operator<<(T (&v)[N])
		{
			append((unsigned char *)v,sizeof(v));
			return *this;
		}


		template<class T>
		ByteBuffer &append(const T& value)
		{
			append((unsigned char *)&value, sizeof(value));
			return *this;
		}

		ByteBuffer& operator<<(const ByteBuffer& buffer)
		{
			append(buffer.contents(),buffer.size());
			return *this;
		}

		ByteBuffer &operator>>(bool &value)
		{
			value = read<char>() > 0 ? true : false;
			return *this;
		}

		template<typename T,size_t N>
		ByteBuffer  &operator>>(T (&v)[N])
		{
			get((unsigned char *)v,sizeof(v));
			return *this;
		}



protected:


		template <typename T>  void read(T & value,size_t pos) const
		{
			if(pos + sizeof(T) > size())
				throw ByteBufferException("read", pos, wpos_, sizeof(T), data_size_);
			value=*((T*)(contents()+pos));
		}
		bool selfmemory() const{
			if (data_ptr_!=NULL || !raw_data_ptr_){
				return true;
			}
			return false;
		}

		unsigned char * dataptr() const{
			return (unsigned char *)raw_data_ptr_;
		};
		
		boost::shared_ptr<unsigned char> data_ptr_;
		
		size_t	rpos_;
		union{
			unsigned int	data_size_;			//数据大小
			size_t	wpos_;				//写位置
		};
        unsigned char *	raw_data_ptr_;
		unsigned int  storage_size_;		//存储空间大小
public:

	//用于设置宽度
	template<int N>
	struct	vec_head_size{
	};
	template <int N> ByteBuffer &operator<<(vec_head_size<N> w){	array_with_bytes_ = N;return *this;}
	template <int N> ByteBuffer &operator>>(vec_head_size<N> w){	array_with_bytes_ = N;return *this;}

	unsigned char	array_with_bytes_;		//在存储的时候，用于表示接下来数组大小所需要的字节数，默认为1

public:
	template<class _Type,class T2,class T3>
	ByteBuffer &operator<<(const std::basic_string<_Type,T2,T3> &s)
	{
		if (array_with_bytes_){
			unsigned int len = s.size();
			append(&len,array_with_bytes_);
			append((unsigned char *)s.c_str(),s.size()*sizeof(_Type));
		}else{
			append((unsigned char *)s.c_str(),s.size()*sizeof(_Type)+sizeof(_Type));
		}
		return *this;
	}


	template<class _Type,class T2,class T3>
	ByteBuffer &operator>>(std::basic_string<_Type,T2,T3>& s)
	{
		s.clear();
		if (array_with_bytes_){
			unsigned int len=0;
			get((unsigned char *)&len,array_with_bytes_);
			s.resize(len);
			get(const_cast<unsigned char*>((const unsigned char *)s.c_str()),len*sizeof(_Type));
		}else{
			while (true)
			{
				_Type c=read<_Type>();
				if (c==0)
					break;
				s+=c;
			}
		}
		return *this;
	}

	inline ByteBuffer &operator<<(const char * s)
	{
		if (array_with_bytes_){
			unsigned int len = strlen(s);
			append(&len,array_with_bytes_);
			append((unsigned char *)s,strlen(s));
		}else{
			append((unsigned char *)s,strlen(s)+1);
		}
		return *this;
	}

	inline ByteBuffer &operator>>(char * s)
	{	
		if (array_with_bytes_){
			unsigned int len=0;
			get((unsigned char *)&len,array_with_bytes_);
			get((unsigned char *)s,len);
			s[len]=0;
		}else{
			while (true)
			{
				*s++=read<char>();
				if (*s==0)
					break;
			}
		}
		return *this;
	}

};

/*
打包一个容器
*/
template<class _RanIt> inline
ByteBuffer & vecpack(_RanIt _First, _RanIt _Last,ByteBuffer &b,unsigned int size_width=BUF_VEC_WIDTH){
	unsigned int bpos = b.wpos();
	size_t num=0 ;
	b.append((unsigned char *)&num,size_width);
	
	for (_RanIt it=_First;it!=_Last;it++){
		b<<*it;
		num++;
	}
	b.put(bpos,&num,size_width);
	return b;
}

/*
解包一个容器
*/
template<typename T>
ByteBuffer & vecdepack(T& container,ByteBuffer &b,unsigned int size_width=BUF_VEC_WIDTH){
	
	unsigned int vsize=0;
	b.get((unsigned char *)&vsize,size_width);
	container.clear();
	container.resize(vsize);
	for (T::iterator it = container.begin();it!=container.end();it++){
		 b>> *it;
	}
	//for (unsigned int i=0;i<vsize;i++){
	//	b>> container[i];
	//}
	return b;
}

template <typename T> ByteBuffer &operator<<(ByteBuffer &b,const std::set<T>& s)
{
	b << (unsigned int)s.size();
	for (typename std::set<T>::const_iterator i = s.begin(); i != s.end(); i++)
	{
		b << *i;
	}
	return b;
}

template <typename T> ByteBuffer &operator>>(ByteBuffer &b, std::set<T> &s)
{
	unsigned int ssize;
	b >> ssize;
	s.clear();
	while(ssize--)
	{
		T t;
		b >> t;
		s.insert(t);
	}
	return b;
}

template <typename T> ByteBuffer &operator<<(ByteBuffer &b,const std::vector<T>& v)
{
    vecpack(v.begin(),v.end(),b,b.array_with_bytes_);
	return b;
}

template <typename T> ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v)
{
   vecdepack(v,b,b.array_with_bytes_);
	return b;
}

template <typename T> ByteBuffer &operator<<(ByteBuffer &b,const std::list<T>& v)
{
     vecpack(v.begin(),v.end(),b,b.array_with_bytes_);
	  return b;
}

template <typename T> ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v)
{
	vecdepack(v,b,b.array_with_bytes_);
	return b;
}


template <typename K, typename V,typename F> ByteBuffer &operator<<(ByteBuffer &b,const std::map<K, V,F> &m)
{
    b << (unsigned int)m.size();
    for (typename std::map<K, V,F>::const_iterator i = m.begin(); i != m.end(); i++)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V,typename F> ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V,F> &m)
{
    unsigned int msize;
    b >> msize;
    m.clear();
    while(msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}
template <typename T1, typename T2> ByteBuffer &operator<<(ByteBuffer &b,const std::pair<T1,T2> &p)
{
	b << p.first<<p.second;
	return b;
}

template <typename T1, typename T2> ByteBuffer &operator>>(ByteBuffer &b, std::pair<T1,T2> &p)
{
	b >> p.first>>p.second;
	return b;
}




//用于生成序列化函数
#define DEC_BUF_MEMCPY(type)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){b.append(t);	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){	b.get((unsigned char *)&t,sizeof(t));	return b;} 
#define DEC_BUF_OP1(type,p1)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1;	return b;} 
#define DEC_BUF_OP2(type,p1,p2)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2;	return b;} 
#define DEC_BUF_OP3(type,p1,p2,p3)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3;	return b;} 
#define DEC_BUF_OP4(type,p1,p2,p3,p4)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4;	return b;} 
#define DEC_BUF_OP5(type,p1,p2,p3,p4,p5)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5;	return b;} 
#define DEC_BUF_OP6(type,p1,p2,p3,p4,p5,p6)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6;	return b;} 
#define DEC_BUF_OP7(type,p1,p2,p3,p4,p5,p6,p7)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7;	return b;} 
#define DEC_BUF_OP8(type,p1,p2,p3,p4,p5,p6,p7,p8)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8;	return b;} 
#define DEC_BUF_OP9(type,p1,p2,p3,p4,p5,p6,p7,p8,p9)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9;	return b;} 
#define DEC_BUF_OP10(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10;	return b;} 
#define DEC_BUF_OP11(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11;	return b;} 
#define DEC_BUF_OP12(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12;	return b;} 
#define DEC_BUF_OP13(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13;	return b;} 
#define DEC_BUF_OP14(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14;	return b;} 
#define DEC_BUF_OP15(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15;	return b;} 
#define DEC_BUF_OP16(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16;	return b;} 
#define DEC_BUF_OP17(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17;	return b;} 
#define DEC_BUF_OP18(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18;	return b;} 
#define DEC_BUF_OP19(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19;	return b;} 
#define DEC_BUF_OP20(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20;	return b;} 
#define DEC_BUF_OP21(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21;	return b;} 
#define DEC_BUF_OP22(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22;	return b;} 
#define DEC_BUF_OP23(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23;	return b;} 
#define DEC_BUF_OP24(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24;	return b;} 
#define DEC_BUF_OP25(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25;	return b;} 
#define DEC_BUF_OP26(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26;	return b;} 
#define DEC_BUF_OP27(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27;	return b;} 
#define DEC_BUF_OP28(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28;	return b;} 
#define DEC_BUF_OP29(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29;	return b;} 
#define DEC_BUF_OP30(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30;	return b;} 
#define DEC_BUF_OP31(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31;	return b;} 
#define DEC_BUF_OP32(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31<<t.p32;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31>>t.p32;	return b;} 
#define DEC_BUF_OP33(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31<<t.p32<<t.p33;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31>>t.p32>>t.p33;	return b;} 
#define DEC_BUF_OP34(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33,p34)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31<<t.p32<<t.p33<<t.p34;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31>>t.p32>>t.p33>>t.p34;	return b;} 
#define DEC_BUF_OP35(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33,p34,p35)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31<<t.p32<<t.p33<<t.p34<<t.p35;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31>>t.p32>>t.p33>>t.p34>>t.p35;	return b;} 
#define DEC_BUF_OP36(type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33,p34,p35,p36)	inline ByteBuffer &operator<<(ByteBuffer &b,const type & t){		b<<t.p1<<t.p2<<t.p3<<t.p4<<t.p5<<t.p6<<t.p7<<t.p8<<t.p9<<t.p10<<t.p11<<t.p12<<t.p13<<t.p14<<t.p15<<t.p16<<t.p17<<t.p18<<t.p19<<t.p20<<t.p21<<t.p22<<t.p23<<t.p24<<t.p25<<t.p26<<t.p27<<t.p28<<t.p29<<t.p30<<t.p31<<t.p32<<t.p33<<t.p34<<t.p35<<t.p36;	return b;} inline ByteBuffer &operator>>(ByteBuffer &b, type & t){		b>>t.p1>>t.p2>>t.p3>>t.p4>>t.p5>>t.p6>>t.p7>>t.p8>>t.p9>>t.p10>>t.p11>>t.p12>>t.p13>>t.p14>>t.p15>>t.p16>>t.p17>>t.p18>>t.p19>>t.p20>>t.p21>>t.p22>>t.p23>>t.p24>>t.p25>>t.p26>>t.p27>>t.p28>>t.p29>>t.p30>>t.p31>>t.p32>>t.p33>>t.p34>>t.p35>>t.p36;	return b;} 

#endif
