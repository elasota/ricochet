#pragma once

#include "CoreDefs.h"
#include "Nothing.h"

namespace rkci
{
	template<class T>
	union Placeholder
	{
		Nothing m_nothing;
		T m_value;

		Placeholder();
		Placeholder(const T &obj);
		Placeholder(T &&obj);
		~Placeholder();

		Placeholder &operator=(const T &obj);
		Placeholder &operator=(T &&obj);
	};
}

template<class T>
rkci::Placeholder<T>::Placeholder()
{
}

template<class T>
rkci::Placeholder<T>::Placeholder(const T &obj)
	: m_value(obj)
{
}

template<class T>
rkci::Placeholder<T>::Placeholder(T &&obj)
	: m_value(rkci::Move(obj))
{
}

template<class T>
rkci::Placeholder<T>::~Placeholder()
{
}

template<class T>
rkci::Placeholder<T> &rkci::Placeholder<T>::operator=(const T &obj)
{
	m_value = obj;
}

template<class T>
rkci::Placeholder<T> &rkci::Placeholder<T>::operator=(T &&obj)
{
	m_value = rkci::Move(obj);
}
