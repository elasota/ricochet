#pragma once

#include "CoreDefs.h"

namespace rkci
{
	namespace HashUtil
	{
		Hash_t ComputePODHash(const void *data, size_t size);
	}

	template<class T>
	class PODHasher
	{
	public:
		static Hash_t Compute(const T &key);
	};

	template<class T>
	class UseDefaultHash
	{
	public:
		static const bool kValue = std::is_arithmetic<T>::value;
	};

	template<class T, bool TIsPOD>
	class DefaultHasher
	{
	};

	template<class T>
	class DefaultHasher<T, true> : public PODHasher<T>
	{
	};


	template<class T>
	class Hasher final : public DefaultHasher<T, UseDefaultHash<T>::kValue>
	{
	public:
	};
}

template<class T>
inline rkci::Hash_t rkci::PODHasher<T>::Compute(const T &key)
{
	return rkci::HashUtil::ComputePODHash(&key, sizeof(key));
}
