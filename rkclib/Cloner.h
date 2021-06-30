#pragma once

#include "CoreDefs.h"
#include "Result.h"

namespace rkci
{
	template<class T>
	class Cloner
	{
	public:
		static rkci::ResultRV<T> Clone(const T &t);
	};
}

template<class T>
rkci::ResultRV<T> rkci::Cloner<T>::Clone(const T &t)
{
	return rkci::ResultRV<T>(T(t));
}
