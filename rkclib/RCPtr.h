#pragma once

namespace rkci
{
	struct IRefCounted;

	template<class T>
	class RCPtr final
	{
	public:
		RCPtr(T *ref);
		RCPtr(const RCPtr<T> &other);
		RCPtr(RCPtr<T> &&other);
		~RCPtr();

		RCPtr<T> &operator=(const RCPtr<T> &other);
		RCPtr<T> &operator=(RCPtr<T> &&other);
		RCPtr<T> &operator=(T *other);

		bool operator==(const T *other) const;
		bool operator!=(const T *other) const;

		operator T*() const;

	private:
		IRefCounted *m_refCounted;
	};
}

#include "RefCounted.h"

template<class T>
rkci::RCPtr<T>::RCPtr(T *ref)
	: m_refCounted(ref)
{
	if (m_refCounted)
		m_refCounted->AddRef();
}

template<class T>
rkci::RCPtr<T>::RCPtr(const RCPtr<T> &other)
	: m_refCounted(other.m_refCounted)
{
	if (m_refCounted)
		m_refCounted->AddRef();
}

template<class T>
rkci::RCPtr<T>::RCPtr(RCPtr<T> &&other)
	: m_refCounted(other.m_refCounted)
{
	other.m_refCounted = nullptr;
}

template<class T>
rkci::RCPtr<T>::~RCPtr()
{
	if (m_refCounted)
		m_refCounted->DecRef();
}

template<class T>
rkci::RCPtr<T> &rkci::RCPtr<T>::operator=(const RCPtr<T> &other)
{
	if (other.m_refCounted)
		other.m_refCounted->AddRef();
	if (m_refCounted)
		m_refCounted->DecRef();

	m_refCounted = other.m_refCounted;
}

template<class T>
rkci::RCPtr<T> &rkci::RCPtr<T>::operator=(RCPtr<T> &&other)
{
	if (m_refCounted)
		m_refCounted->DecRef();

	m_refCounted = other.m_refCounted;
	other.m_refCounted = nullptr;
}

template<class T>
rkci::RCPtr<T> &rkci::RCPtr<T>::operator=(T *other)
{
	if (other)
	{
		IRefCounted *otherRC = other;
		otherRC->AddRef();
	}

	if (m_refCounted)
		m_refCounted->DecRef();

	m_refCounted = other;
}

template<class T>
bool rkci::RCPtr<T>::operator==(const T *other) const
{
	return m_refCounted == other;
}

template<class T>
bool rkci::RCPtr<T>::operator!=(const T *other) const
{
	return m_refCounted != other;
}

template<class T>
rkci::RCPtr<T>::operator T*() const
{
	return static_cast<T*>(m_refCounted);
}
