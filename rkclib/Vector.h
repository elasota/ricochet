#pragma once

#include "CoreDefs.h"
#include "Result.h"

#include <stdint.h>

namespace rkci
{
	struct IAllocator;

	template<class T> class ArraySliceView;

	template<class T, size_t TStaticSize>
	class VectorStaticData
	{
	protected:
		alignas(alignof(T)) uint8_t m_staticElementData[sizeof(T) * TStaticSize];

		T *GetStaticElements();
		const T *GetStaticElements() const;
	};

	template<class T>
	class VectorStaticData<T, 0>
	{
	protected:
		T *GetStaticElements();
		const T *GetStaticElements() const;
	};

	template<class T, size_t TStaticSize = 0>
	class Vector : public VectorStaticData<T, TStaticSize>
	{
	public:
		explicit Vector(IAllocator *alloc);
		Vector(Vector<T, TStaticSize> &&other);
		~Vector();

		T &operator[](size_t index);
		const T &operator[](size_t index) const;

		void Optimize();
		Result Resize(size_t newSize);
		void ResizeStatic(size_t newSize);
		Result Reserve(size_t newSize);

		// These resize the buffer and expand capacity, but do not construct any new elements
		Result ResizeNoConstruct(size_t newSize);
		void ResizeNoConstructStatic(size_t newSize);

		Result Append(const T &item);
		Result Append(T &&item);

		ArraySliceView<T> Slice();
		ArraySliceView<const T> Slice() const;

		const size_t Count() const;

		ResultRV<Vector<T, TStaticSize>> Clone() const;

		Vector<T, TStaticSize> &operator=(Vector<T, TStaticSize> &&other);

		IAllocator *GetAllocator() const;

	private:
		Vector(const Vector<T, TStaticSize> &other) = delete;

		static const size_t kStaticSize = TStaticSize;

		T *m_elements;
		size_t m_capacity;
		size_t m_count;
		IAllocator *m_alloc;
	};
}


#include <new>
#include <cassert>
#include "ArraySliceView.h"
#include "Cloner.h"
#include "IAllocator.h"

namespace rkci
{
	template<class T, size_t TStaticSize>
	class Cloner<Vector<T, TStaticSize>>
	{
	public:
		static rkci::ResultRV<Vector<T, TStaticSize>> Clone(const Vector<T, TStaticSize> &t);
	};
}


template<class T, size_t TStaticSize>
static rkci::ResultRV<rkci::Cloner<rkci::Vector<T, TStaticSize>>> Clone(const rkci::Vector<T, TStaticSize> &t)
{
	return t.Clone();
}

template<class T, size_t TStaticSize>
T *rkci::VectorStaticData<T, TStaticSize>::GetStaticElements()
{
	return reinterpret_cast<T*>(this->m_staticElementData);
}

template<class T, size_t TStaticSize>
const T *rkci::VectorStaticData<T, TStaticSize>::GetStaticElements() const
{
	return reinterpret_cast<const T*>(this->m_staticElementData);
}

template<class T>
T *rkci::VectorStaticData<T, 0>::GetStaticElements()
{
	return nullptr;
}

template<class T>
const T *rkci::VectorStaticData<T, 0>::GetStaticElements() const
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// GpVector
template<class T, size_t TStaticSize>
rkci::Vector<T, TStaticSize>::Vector(IAllocator *alloc)
	: m_elements(this->GetStaticElements())
	, m_capacity(TStaticSize)
	, m_count(0)
	, m_alloc(alloc)
{
}

template<class T, size_t TStaticSize>
rkci::Vector<T, TStaticSize>::Vector(Vector<T, TStaticSize>&& other)
	: m_elements(other.m_elements)
	, m_capacity(other.m_capacity)
	, m_count(other.m_count)
	, m_alloc(other.m_alloc)
{
	if (m_capacity <= TStaticSize)
	{
		const size_t count = m_count;
		T *elements = this->GetStaticElements();
		const T *srcElements = other.m_elements;
		m_elements = elements;
		for (size_t i = 0; i < count; i++)
			new (m_elements + i) T(static_cast<T&&>(other.m_elements[i]));
	}

	other.m_count = 0;
	other.m_capacity = TStaticSize;
	other.m_elements = other.GetStaticElements();
}

template<class T, size_t TStaticSize>
rkci::Vector<T, TStaticSize>::~Vector()
{
	T *elements = m_elements;
	size_t remaining = m_count;

	while (remaining > 0)
	{
		remaining--;
		elements[remaining].~T();
	}

	if (m_capacity > TStaticSize)
	{
		m_alloc->Release(m_elements);
	}
}

template<class T, size_t TStaticSize>
T &rkci::Vector<T, TStaticSize>::operator[](size_t index)
{
	assert(index <= m_count);
	return m_elements[index];
}

template<class T, size_t TStaticSize>
const T &rkci::Vector<T, TStaticSize>::operator[](size_t index) const
{
	assert(index <= m_count);
	return m_elements[index];
}

template<class T, size_t TStaticSize>
void rkci::Vector<T, TStaticSize>::Optimize()
{
	if (m_capacity <= TStaticSize || m_capacity == m_count)
		return;

	const size_t count = m_count;
	T *elements = m_elements;

	T *newElements = nullptr;
	if (m_count <= TStaticSize)
	{
		newElements = this->GetStaticElements();
		m_capacity = TStaticSize;
	}
	else
	{
		newElements = static_cast<T*>(m_alloc->Alloc(sizeof(T) * m_count));
		if (!newElements)
			return;

		m_capacity = m_count;
	}

	for (size_t i = 0; i < count; i++)
		new (newElements + i) T(rkci::Move(elements[i]));

	for (size_t i = 0; i < count; i++)
		elements[i].~T();

	m_alloc->Release(elements);
	m_elements = newElements;
}


template<class T, size_t TStaticSize>
rkci::Result rkci::Vector<T, TStaticSize>::Resize(size_t newSize)
{
	const size_t oldCount = m_count;

	RKC_CHECK(ResizeNoConstruct(newSize));

	for (size_t i = oldCount; i < newSize; i++)
		new (m_elements + i) T();

	return Result::Ok();
}

template<class T, size_t TStaticSize>
void rkci::Vector<T, TStaticSize>::ResizeStatic(size_t newSize)
{
	const size_t oldCount = m_count;

	ResizeNoConstructStatic(newSize);

	for (size_t i = oldCount; i < newSize; i++)
		new (m_elements + i) T();
}

template<class T, size_t TStaticSize>
rkci::Result rkci::Vector<T, TStaticSize>::Reserve(size_t newSize)
{
	const size_t oldCount = m_count;

	RKC_CHECK(ResizeNoConstruct(newSize));

	m_count = oldCount;

	return Result::Ok();
}

template<class T, size_t TStaticSize>
rkci::Result rkci::Vector<T, TStaticSize>::ResizeNoConstruct(size_t newSize)
{
	T *elements = m_elements;

	if (newSize <= m_count)
	{
		size_t count = m_count;
		while (count > newSize)
		{
			count--;
			m_elements[count].~T();
		}

		m_count = count;
		return Result::Ok();
	}

	if (newSize <= m_capacity)
	{
		m_count = newSize;
		return Result::Ok();
	}

	size_t newCapacity = newSize;
	assert(newCapacity > kStaticSize);

	T *newElements = static_cast<T*>(m_alloc->Alloc(newCapacity * sizeof(T)));
	if (!newElements)
		return ::rkc::ResultCodes::kOutOfMemory;

	const size_t oldCount = m_count;
	for (size_t i = 0; i < oldCount; i++)
		new (newElements + i) T(static_cast<T&&>(elements[i]));

	for (size_t i = 0; i < oldCount; i++)
		elements[oldCount - 1 - i].~T();

	if (m_capacity > kStaticSize)
		m_alloc->Release(m_elements);

	m_elements = newElements;
	m_capacity = newCapacity;
	m_count = newSize;

	return Result::Ok();
}

template<class T, size_t TStaticSize>
void rkci::Vector<T, TStaticSize>::ResizeNoConstructStatic(size_t newSize)
{
	RKC_ASSERT(newSize <= kStaticSize);

	T *staticElements = this->GetStaticElements();
	T *oldElements = m_elements;
	if (oldElements != staticElements)
	{
		const size_t countToMove = newSize;
		for (size_t i = 0; i < countToMove; i++)
			new (staticElements) T(static_cast<T&&>(oldElements[i]));

		for (size_t i = 0; i < m_count; i++)
			oldElements[i].~T();

		m_alloc->Release(oldElements);
		m_elements = staticElements;
	}
	else
	{
		for (size_t i = newSize; i < m_count; i++)
			staticElements[i].~T();
	}

	m_count = newSize;
	m_capacity = kStaticSize;
}

template<class T, size_t TStaticSize>
rkci::Result rkci::Vector<T, TStaticSize>::Append(const T &item)
{
	const size_t oldCount = m_count;

	if (m_count == m_capacity)
	{
		size_t newCapacity = m_capacity * 2;
		if (newCapacity < 8)
			newCapacity = 8;

		RKC_CHECK(Reserve(newCapacity));
	}

	RKC_CHECK(ResizeNoConstruct(oldCount + 1));

	new (m_elements + oldCount) T(item);

	return Result::Ok();
}

template<class T, size_t TStaticSize>
rkci::Result rkci::Vector<T, TStaticSize>::Append(T &&item)
{
	const size_t oldCount = m_count;

	if (m_count == m_capacity)
	{
		size_t newCapacity = m_capacity * 2;
		if (newCapacity < 8)
			newCapacity = 8;

		RKC_CHECK(Reserve(newCapacity));
	}

	RKC_CHECK(ResizeNoConstruct(oldCount + 1));

	new (m_elements + oldCount) T(static_cast<T&&>(item));

	return Result::Ok();
}


template<class T, size_t TStaticSize>
const size_t rkci::Vector<T, TStaticSize>::Count() const
{
	return m_count;
}

template<class T, size_t TStaticSize>
rkci::ArraySliceView<T> rkci::Vector<T, TStaticSize>::Slice()
{
	return ArraySliceView<T>(m_elements, m_count);
}

template<class T, size_t TStaticSize>
rkci::ArraySliceView<const T> rkci::Vector<T, TStaticSize>::Slice() const
{
	return ArraySliceView<const T>(m_elements, m_count);
}

template<class T, size_t TStaticSize>
rkci::ResultRV<rkci::Vector<T, TStaticSize>> rkci::Vector<T, TStaticSize>::Clone() const
{
	const size_t count = m_count;

	rkci::Vector<T, TStaticSize> clone(m_alloc);
	RKC_CHECK(clone.Reserve(count));

	for (size_t i = 0; i < count; i++)
	{
		RKC_CHECK_RV(T, elementClone, rkci::Cloner<T>::Clone(m_elements[i]));
		RKC_CHECK(clone.Append(rkci::Move(elementClone)));
	}

	return clone;
}

template<class T, size_t TStaticSize>
rkci::Vector<T, TStaticSize> &rkci::Vector<T, TStaticSize>::operator=(rkci::Vector<T, TStaticSize> &&other)
{
	if (this == &other)
		return *this;

	const size_t oldCount = m_count;
	for (size_t i = 0; i < oldCount; i++)
		m_elements[i].~T();

	const size_t oldCapacity = m_capacity;
	m_capacity = other.m_capacity;
	m_count = other.m_count;
	m_alloc = other.m_alloc;

	if (oldCapacity > TStaticSize)
	{
		RKC_ASSERT(m_elements != this->GetStaticElements());
		m_alloc->Release(m_elements);
	}

	if (m_capacity > TStaticSize)
		m_elements = other.m_elements;
	else
	{
		const size_t newCount = other.m_count;
		m_elements = this->GetStaticElements();
		T *otherElements = other.m_elements;
		for (size_t i = 0; i < newCount; i++)
			new (m_elements + i) T(rkci::Move(otherElements[i]));
	}

	other.m_count = 0;
	other.m_capacity = TStaticSize;
	other.m_elements = other.GetStaticElements();

	return *this;
}


template<class T, size_t TStaticSize>
rkci::IAllocator *rkci::Vector<T, TStaticSize>::GetAllocator() const
{
	return m_alloc;
}
