#pragma once

namespace rkci
{
	template<class T> class ArraySliceView;

	namespace ArrayTools
	{
		template<class T>
		void Move(ArraySliceView<T> dest, ArraySliceView<T> src);
	}
}

#include "ArraySliceView.h"

template<class T>
void rkci::ArrayTools::Move(ArraySliceView<T> dest, ArraySliceView<T> src)
{
	const size_t count = src.Count();
	RKC_ASSERT(count <= dest.Count());

	if (count == 0)
		return;

	T *destFirst = &dest[0];
	T *destLast = &dest[count - 1];

	T *srcFirst = &src[0];
	T *srcLast = &src[count - 1];

	if (srcFirst == destFirst)
		return;

	if (destFirst >= srcFirst && destFirst <= srcLast)
	{
		// Ascending order write will overlap
		for (size_t i = 0; i < count; i++)
			destFirst[count - 1 - i] = static_cast<T&&>(srcFirst[count - 1 - i]);
	}
	else
	{
		for (size_t i = 0; i < count; i++)
			destFirst[i] = static_cast<T&&>(srcFirst[i]);
	}
}
