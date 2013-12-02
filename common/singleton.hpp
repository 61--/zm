#pragma once

//����ģʽ
template <typename T>
struct Singleton
{
protected:
	Singleton(){};
	~Singleton(){};

public:
	typedef T object_type;

	// ȡ��ʵ��
	// ʵ���ڵ�һ�ε���instanceʱ������
	static object_type & Instance()
	{
		static object_type obj;

		return obj;
	}
};