#pragma once

#include "CoreDefs.h"

namespace rkci
{
	template<class T> class ArraySliceView;

	template<class T, size_t TSize>
	class StaticArray
	{
	public:
		StaticArray();

		T &operator[](size_t index);
		const T &operator[](size_t index) const;

		ArraySliceView<const T> GetSlice() const;
		ArraySliceView<T> GetSlice();

		static size_t Count();

	private:
		T m_array[TSize];
	};
}

#include "ArraySliceView.h"

template<class T, size_t TSize>
rkci::StaticArray<T, TSize>::StaticArray()
{
}

template<class T, size_t TSize>
T &rkci::StaticArray<T, TSize>::operator[](size_t index)
{
	RKC_ASSERT(index < TSize);
	return m_array[index];
}

template<class T, size_t TSize>
const T &rkci::StaticArray<T, TSize>::operator[](size_t index) const
{
	RKC_ASSERT(index < TSize);
	return m_array[index];
}

template<class T, size_t TSize>
rkci::ArraySliceView<const T> rkci::StaticArray<T, TSize>::GetSlice() const
{
	return rkci::ArraySliceView<const T>(m_array, TSize);
}

template<class T, size_t TSize>
rkci::ArraySliceView<T> rkci::StaticArray<T, TSize>::GetSlice()
{
	return rkci::ArraySliceView<T>(m_array, TSize);
}

template<class T, size_t TSize>
size_t rkci::StaticArray<T, TSize>::Count()
{
	return TSize;
}
