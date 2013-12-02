#pragma once

//单件模式
template <typename T>
struct Singleton
{
protected:
	Singleton(){};
	~Singleton(){};

public:
	typedef T object_type;

	// 取得实例
	// 实例在第一次调用instance时被创建
	static object_type & Instance()
	{
		static object_type obj;

		return obj;
	}
};