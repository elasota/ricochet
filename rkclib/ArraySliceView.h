#pragma once

#include "CoreDefs.h"

namespace rkci
{
	template<class T>
	class ArraySliceViewIterator
	{
	public:
		explicit ArraySliceViewIterator(T *buffer, size_t offset, size_t count);

		bool operator==(const ArraySliceViewIterator<T> &other) const;
		bool operator!=(const ArraySliceViewIterator<T> &other) const;

		T operator*() const;

		ArraySliceViewIterator<T> &operator++();
		ArraySliceViewIterator<T> operator++(int);

		T &operator[](size_t index) const;
		T &operator[](ptrdiff_t index) const;

		operator ArraySliceViewIterator<const T>() const;

	private:
		explicit ArraySliceViewIterator(T *buffer, ptrdiff_t numBefore, ptrdiff_t numAfter);

		T *m_buffer;
		ptrdiff_t m_numBefore;
		ptrdiff_t m_numAfter;
	};

	template<class T>
	class ArraySliceView
	{
	public:
#if RKC_IS_DEBUG
		typedef ArraySliceViewIterator<T> Iterator_t;
#else
		typedef T *Iterator_t;
		typedef const T *ConstIterator_t;
#endif

		ArraySliceView();
		ArraySliceView(T *buffer, size_t count);

		T &operator[](size_t index) const;

		ArraySliceView<T> Subrange(size_t startIndex, size_t count) const;

		ArraySliceView<const T> ToConst() const;
		operator ArraySliceView<const T>() const;

		Iterator_t begin() const;
		Iterator_t end() const;

		size_t Count() const;

	private:
		T *m_buffer;
		size_t m_count;
	};
}

template<class T>
rkci::ArraySliceViewIterator<T>::ArraySliceViewIterator(T *buffer, size_t offset, size_t count)
	: m_buffer(buffer)
	, m_numAfter(static_cast<ptrdiff_t>(count - offset))
	, m_numBefore(static_cast<ptrdiff_t>(offset))
{
	RKC_ASSERT(offset <= count);
}

template<class T>
rkci::ArraySliceViewIterator<T>::ArraySliceViewIterator(T *buffer, ptrdiff_t numBefore, ptrdiff_t numAfter)
	: m_buffer(buffer)
	, m_numBefore(numBefore)
	, m_numAfter(numAfter)
{
}

template<class T>
bool rkci::ArraySliceViewIterator<T>::operator==(const ArraySliceViewIterator<T> &other) const
{
	if (m_buffer == other.m_buffer && m_numBefore == other.m_numBefore)
	{
		RKC_ASSERT(m_numAfter == other.m_numAfter);
		return true;
	}
	return false;
}

template<class T>
bool rkci::ArraySliceViewIterator<T>::operator!=(const ArraySliceViewIterator<T> &other) const
{
	return !((*this) == other);
}


template<class T>
T rkci::ArraySliceViewIterator<T>::operator*() const
{
	return m_buffer[m_numBefore];
}


template<class T>
rkci::ArraySliceViewIterator<T> &rkci::ArraySliceViewIterator<T>::operator++()
{
	RKC_ASSERT(m_numAfter);
	m_numAfter--;
	m_numBefore++;
	return *this;
}

template<class T>
rkci::ArraySliceViewIterator<T> rkci::ArraySliceViewIterator<T>::operator++(int)
{
	rkci::ArraySliceViewIterator<T> clone(*this);

	RKC_ASSERT(m_numAfter);
	m_numAfter--;
	m_numBefore++;

	return clone;
}

template<class T>
T &rkci::ArraySliceViewIterator<T>::operator[](size_t index) const
{
	RKC_ASSERT(index < PTRDIFF_MAX);
	return this->operator*[static_cast<ptrdiff_t>(index)];
}

template<class T>
T &rkci::ArraySliceViewIterator<T>::operator[](ptrdiff_t index) const
{
	RKC_ASSERT(index < m_numAfter);
	RKC_ASSERT(index >= -m_numBefore);
	return m_buffer[m_numBefore + index];
}


template<class T>
rkci::ArraySliceView<T>::ArraySliceView()
	: m_buffer(nullptr)
	, m_count(0)
{
}
template<class T>
rkci::ArraySliceView<T>::ArraySliceView(T *buffer, size_t count)
	: m_buffer(buffer)
	, m_count(count)
{
}

template<class T>
T &rkci::ArraySliceView<T>::operator[](size_t index) const
{
	RKC_ASSERT(m_buffer != nullptr);
	RKC_ASSERT(index < m_count);
	return m_buffer[index];
}

template<class T>
rkci::ArraySliceViewIterator<T>::operator ArraySliceViewIterator<const T>() const
{
	return ArraySliceViewIterator<const T>(m_buffer, m_numBefore, m_numAfter);
}


template<class T>
rkci::ArraySliceView<T> rkci::ArraySliceView<T>::Subrange(size_t startIndex, size_t count) const
{
	RKC_ASSERT(m_buffer != nullptr);
	RKC_ASSERT(startIndex <= m_count);
	RKC_ASSERT(m_count - startIndex >= count);
	return ArraySliceView<T>(m_buffer + startIndex, count);
}

template<class T>
rkci::ArraySliceView<const T> rkci::ArraySliceView<T>::ToConst() const
{
	return ArraySliceView<const T>(m_buffer, m_count);
}

template<class T>
rkci::ArraySliceView<T>::operator ArraySliceView<const T>() const
{
	return ArraySliceView<const T>(m_buffer, m_count);
}

template<class T>
size_t rkci::ArraySliceView<T>::Count() const
{
	return m_count;
}


template<class T>
typename rkci::ArraySliceView<T>::Iterator_t rkci::ArraySliceView<T>::begin() const
{
	return rkci::ArraySliceViewIterator<T>(m_buffer, 0, m_count);
}

template<class T>
typename rkci::ArraySliceView<T>::Iterator_t rkci::ArraySliceView<T>::end() const
{
	return rkci::ArraySliceViewIterator<T>(m_buffer, m_count, m_count);
}
