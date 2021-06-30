#pragma once

#include "CoreDefs.h"

namespace rkci
{
	struct IAllocator;
	class Result;

	template<class T> class Hasher;
	template<class T> class Comparer;
	template<class T> class Optional;

	template<class TKey, class TValue> class HashMap;
	template<class TKey, class TValue> class HashMapConstIterator;
	template<class TKey, class TValue> class HashMapIterator;

	namespace HashMapUtils
	{
		enum class CompactValuePrecision
		{
			kUInt8,
			kUInt16,
			kUInt32,
			kUInt64,
		};

		size_t GetCompactValue(const size_t *items, CompactValuePrecision cvPrecision, size_t index);
		void SetCompactValue(size_t *items, CompactValuePrecision cvPrecision, size_t index, size_t value);
		size_t GetMainPosition(Hash_t hash, size_t count);
	}

	template<class TKey, class TValue>
	class KeyValuePairView
	{
	public:
		explicit KeyValuePairView(TKey &key, TValue &value);

		TKey &Key() const;
		TValue &Value() const;

	private:
		TKey &m_key;
		TValue &m_value;
	};

	template<class TKey, class TValue>
	class HashMapConstIterator
	{
	public:
		friend class HashMap<TKey, TValue>;
		friend class HashMapIterator<TKey, TValue>;

		HashMapConstIterator(const HashMapIterator<TKey, TValue> &mutableIterator);

		const TKey &Key() const;
		const TValue &Value() const;

		bool operator==(const HashMapConstIterator<TKey, TValue> &other) const;
		bool operator!=(const HashMapConstIterator<TKey, TValue> &other) const;

		KeyValuePairView<const TKey, const TValue> operator*() const;

		HashMapConstIterator<TKey, TValue> &operator++();
		HashMapConstIterator<TKey, TValue> operator++(int);

	private:
		explicit HashMapConstIterator(const HashMap<TKey, TValue> &hashMap, size_t offset);

		const HashMap<TKey, TValue> &m_hashMap;
		size_t m_offset;
	};

	template<class TKey, class TValue>
	class HashMapIterator
	{
	public:
		friend class HashMap<TKey, TValue>;
		friend class HashMapConstIterator<TKey, TValue>;

		const TKey &Key() const;
		TValue &Value() const;

		bool operator==(const HashMapIterator<TKey, TValue> &other) const;
		bool operator!=(const HashMapIterator<TKey, TValue> &other) const;

		KeyValuePairView<const TKey, TValue> operator*() const;

		HashMapIterator<TKey, TValue> &operator++();
		HashMapIterator<TKey, TValue> operator++(int);

	private:
		explicit HashMapIterator(const HashMap<TKey, TValue> &hashMap, size_t offset);

		const HashMap<TKey, TValue> &m_hashMap;
		size_t m_offset;
	};

	template<class TKey, class TValue>
	class HashMap
	{
	public:
		friend class HashMapIterator<TKey, TValue>;
		friend class HashMapConstIterator<TKey, TValue>;

		explicit HashMap(IAllocator &alloc);
		~HashMap();

		Result Insert(const TKey &key, const TValue &value);
		Result Insert(TKey &&key, const TValue &value);
		Result Insert(const TKey &key, TValue &&value);
		Result Insert(TKey &&key, TValue &&value);

		template<class TKeyCandidate>
		HashMapConstIterator<TKey, TValue> Find(const TKeyCandidate &keyCandidate) const;

		template<class TKeyCandidate>
		HashMapIterator<TKey, TValue> Find(const TKeyCandidate &keyCandidate);

		template<class TKeyCandidate>
		bool Remove(const TKeyCandidate &keyCandidate);

		void Remove(const HashMapIterator<TKey, TValue> &iterator);

		HashMapConstIterator<TKey, TValue> begin() const;
		HashMapIterator<TKey, TValue> begin();

		HashMapConstIterator<TKey, TValue> end() const;
		HashMapIterator<TKey, TValue> end();

	private:
		IAllocator &m_alloc;

		void *m_buffer;
		TKey *m_keys;
		TValue *m_values;
		Hash_t *m_hashes;
		size_t *m_valueMainPosPlusOne;
		size_t *m_nextPlusOne;

		size_t m_capacity;
		size_t m_used;
		size_t m_freeSlotScan;
		HashMapUtils::CompactValuePrecision m_cvPrecision;

		Result Rehash(size_t size);
		Result AutoRehash();

		Result InsertNew(TKey &&key, TValue &&value, Hash_t keyHash, size_t keyMainPosition, size_t mpValueMPPlusOne, bool mayResize);

		template<class TKeyCandidate>
		size_t FindIndex(const TKeyCandidate &keyCandidate);

		void RemoveIndex(size_t index);

		template<class TCandidateKey>
		Optional<size_t> FindKey(const TCandidateKey &key) const;
	};
}

#include "Result.h"
#include "Optional.h"
#include "Hasher.h"
#include "Comparer.h"
#include "Cloner.h"
#include <new>



template<class TKey, class TValue>
rkci::KeyValuePairView<TKey, TValue>::KeyValuePairView(TKey &key, TValue &value)
	: m_key(key)
	, m_value(value)
{
}

template<class TKey, class TValue>
TKey &rkci::KeyValuePairView<TKey, TValue>::Key() const
{
	return m_key;
}

template<class TKey, class TValue>
TValue &rkci::KeyValuePairView<TKey, TValue>::Value() const
{
	return m_value;
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue>::HashMapConstIterator(const HashMap<TKey, TValue> &hashMap, size_t offset)
	: m_hashMap(hashMap)
	, m_offset(offset)
{
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue>::HashMapConstIterator(const HashMapIterator<TKey, TValue> &mutableIterator)
	: m_hashMap(mutableIterator.m_hashMap)
	, m_offset(mutableIterator.m_offset)
{
}

template<class TKey, class TValue>
const TKey &rkci::HashMapConstIterator<TKey, TValue>::Key() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return m_hashMap.m_keys[m_offset];
}

template<class TKey, class TValue>
const TValue &rkci::HashMapConstIterator<TKey, TValue>::Value() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return m_hashMap.m_values[m_offset];
}

template<class TKey, class TValue>
bool rkci::HashMapConstIterator<TKey, TValue>::operator==(const HashMapConstIterator<TKey, TValue> &other) const
{
	return (&m_hashMap == &other.m_hashMap) && (m_offset == other.m_offset);
}

template<class TKey, class TValue>
bool rkci::HashMapConstIterator<TKey, TValue>::operator!=(const HashMapConstIterator<TKey, TValue> &other) const
{
	return !((*this) == other);
}

template<class TKey, class TValue>
rkci::KeyValuePairView<const TKey, const TValue> rkci::HashMapConstIterator<TKey, TValue>::operator*() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return KeyValuePairView<const TKey, const TValue>(m_hashMap.m_keys[m_offset], m_hashMap.m_values[m_offset]);
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue> &rkci::HashMapConstIterator<TKey, TValue>::operator++()
{
	RKC_ASSERT(m_offset < m_hashMap.m_capacity);
	do
	{
		m_offset++;
	} while (m_offset < m_hashMap.m_capacity && m_hashMap.m_valueMainPosPlusOne[m_offset] == 0);
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue> rkci::HashMapConstIterator<TKey, TValue>::operator++(int)
{
	rkci::HashMapConstIterator<TKey, TValue> copy(*this);
	++(*this);
	return copy;
}

template<class TKey, class TValue>
rkci::HashMapIterator<TKey, TValue>::HashMapIterator(const HashMap<TKey, TValue> &hashMap, size_t offset)
	: m_hashMap(hashMap)
	, m_offset(offset)
{
}

template<class TKey, class TValue>
const TKey &rkci::HashMapIterator<TKey, TValue>::Key() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return m_hashMap.m_keys[m_offset];
}

template<class TKey, class TValue>
TValue &rkci::HashMapIterator<TKey, TValue>::Value() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return m_hashMap.m_values[m_offset];
}

template<class TKey, class TValue>
bool rkci::HashMapIterator<TKey, TValue>::operator==(const HashMapIterator<TKey, TValue> &other) const
{
	return (&m_hashMap == &other.m_hashMap) && (m_offset == other.m_offset);
}

template<class TKey, class TValue>
bool rkci::HashMapIterator<TKey, TValue>::operator!=(const HashMapIterator<TKey, TValue> &other) const
{
	return !((*this) == other);
}

template<class TKey, class TValue>
rkci::KeyValuePairView<const TKey, TValue> rkci::HashMapIterator<TKey, TValue>::operator*() const
{
	RKC_ASSERT(m_offset <= m_hashMap.m_capacity);
	RKC_ASSERT(m_hashMap.m_valueMainPosPlusOne[m_offset] != 0);
	return KeyValuePairView<const TKey, TValue>(m_hashMap.m_keys[m_offset], m_hashMap.m_values[m_offset]);
}

template<class TKey, class TValue>
rkci::HashMapIterator<TKey, TValue> &rkci::HashMapIterator<TKey, TValue>::operator++()
{
	RKC_ASSERT(m_offset < m_hashMap.m_capacity);
	do
	{
		m_offset++;
	} while (m_offset < m_hashMap.m_capacity && m_hashMap.m_valueMainPosPlusOne[m_offset] == 0);

	return *this;
}

template<class TKey, class TValue>
rkci::HashMapIterator<TKey, TValue> rkci::HashMapIterator<TKey, TValue>::operator++(int)
{
	rkci::HashMapIterator<TKey, TValue> copy(*this);
	++(*this);
	return copy;
}

template<class TKey, class TValue>
rkci::HashMap<TKey, TValue>::HashMap(IAllocator &alloc)
	: m_buffer(nullptr)
	, m_alloc(alloc)
	, m_keys(nullptr)
	, m_values(nullptr)
	, m_valueMainPosPlusOne(nullptr)
	, m_nextPlusOne(nullptr)
	, m_capacity(0)
	, m_used(0)
	, m_freeSlotScan(0)
	, m_cvPrecision(HashMapUtils::CompactValuePrecision::kUInt8)
{
}

template<class TKey, class TValue>
rkci::HashMap<TKey, TValue>::~HashMap()
{
	const size_t capacity = m_capacity;
	TKey *keys = m_keys;
	TValue *values = m_values;
	const size_t *valueMainPosPlusOne = m_valueMainPosPlusOne;
	HashMapUtils::CompactValuePrecision cvPrecision = m_cvPrecision;

	for (size_t i = 0; i < capacity; i++)
	{
		if (HashMapUtils::GetCompactValue(valueMainPosPlusOne, cvPrecision, i))
		{
			keys[i].~TKey();
			values[i].~TValue();
		}
	}
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::Insert(const TKey &key, const TValue &value)
{
	RKC_CHECK_RV(TKey, clonedKey, Cloner<TKey>::Clone(key));
	RKC_CHECK_RV(TValue, clonedValue, Cloner<TValue>::Clone(value));

	return Insert(rkci::Move(clonedKey), rkci::Move(clonedValue));
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::Insert(TKey &&key, const TValue &value)
{
	RKC_CHECK_RV(TValue, clonedValue, Cloner<TValue>::Clone(value));

	return Insert(rkci::Move(key), rkci::Move(clonedValue));
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::Insert(const TKey &key, TValue &&value)
{
	RKC_CHECK_RV(TKey, clonedKey, Cloner<TKey>::Clone(key));

	return Insert(rkci::Move(clonedKey), rkci::Move(value));
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::Insert(TKey &&key, TValue &&value)
{
	if (m_capacity == 0)
	{
		RKC_CHECK(Rehash(8));
	}

	const Hash_t keyHash = Hasher<TKey>::Compute(key);
	const size_t keyMainPosition = HashMapUtils::GetMainPosition(keyHash, m_capacity);
	const size_t mpValueMPPlusOne = HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, keyMainPosition);

	// Find existing key
	{
		size_t index = keyMainPosition;
		size_t valueMPPlusOne = mpValueMPPlusOne;
		for (;;)
		{
			if (valueMPPlusOne != 0 && Comparer<TKey>::StrictlyEqual(m_keys[index], key))
			{
				m_values[index] = value;
				HashMapUtils::SetCompactValue(m_valueMainPosPlusOne, m_cvPrecision, index, keyMainPosition + 1);
				return Result::Ok();
			}

			const size_t nextIndexPlusOne = HashMapUtils::GetCompactValue(m_nextPlusOne, m_cvPrecision, index);
			if (nextIndexPlusOne == 0)
				break;

			index = nextIndexPlusOne - 1;
			valueMPPlusOne = HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, index);
		}
	}

	return InsertNew(rkci::Move(key), rkci::Move(value), keyHash, keyMainPosition, mpValueMPPlusOne, true);
}



template<class TKey, class TValue>
template<class TKeyCandidate>
rkci::HashMapConstIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::Find(const TKeyCandidate &keyCandidate) const
{
	return rkci::HashMapConstIterator<TKey, TValue>(*this, FindIndex(keyCandidate));
}

template<class TKey, class TValue>
template<class TKeyCandidate>
rkci::HashMapIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::Find(const TKeyCandidate &keyCandidate)
{
	return rkci::HashMapIterator<TKey, TValue>(*this, FindIndex(keyCandidate));
}

template<class TKey, class TValue>
template<class TKeyCandidate>
bool rkci::HashMap<TKey, TValue>::Remove(const TKeyCandidate &keyCandidate)
{
	const size_t index = FindIndex<TKeyCandidate>(keyCandidate);
	if (index != m_capacity)
	{
		this->RemoveIndex(index);
		return true;
	}
	else
		return false;
}

template<class TKey, class TValue>
void rkci::HashMap<TKey, TValue>::Remove(const HashMapIterator<TKey, TValue> &iterator)
{
	RKC_ASSERT(this == &iterator.m_hashMap);

	this->RemoveIndex(iterator.m_offset);
}

template<class TKey, class TValue>
void rkci::HashMap<TKey, TValue>::RemoveIndex(size_t index)
{
	RKC_ASSERT(index < m_capacity);
	RKC_ASSERT(m_valueMainPosPlusOne[index] != 0);

	m_keys[index].~TKey();
	m_values[index].~TValue();
	m_valueMainPosPlusOne[index] = 0;
	m_used--;
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::begin() const
{
	for (size_t i = 0; i < m_capacity; i++)
	{
		if (m_valueMainPosPlusOne[i] != 0)
			return HashMapConstIterator<TKey, TValue>(*this, i);
	}

	return this->end();
}

template<class TKey, class TValue>
rkci::HashMapIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::begin()
{
	for (size_t i = 0; i < m_capacity; i++)
	{
		if (m_valueMainPosPlusOne[i] != 0)
			return HashMapIterator<TKey, TValue>(*this, i);
	}

	return this->end();
}

template<class TKey, class TValue>
rkci::HashMapConstIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::end() const
{
	return HashMapConstIterator<TKey, TValue>(*this, m_capacity);
}

template<class TKey, class TValue>
rkci::HashMapIterator<TKey, TValue> rkci::HashMap<TKey, TValue>::end()
{
	return HashMapIterator<TKey, TValue>(*this, m_capacity);
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::Rehash(size_t size)
{
	const size_t keysPos = 0;
	size_t valuesPos = keysPos + sizeof(TKey) * size;
	valuesPos += alignof(TValue) - 1;
	valuesPos -= valuesPos % alignof(TValue);

	size_t hashesPos = valuesPos + sizeof(TValue) * size;
	hashesPos += alignof(Hash_t) - 1;
	hashesPos -= hashesPos % alignof(Hash_t);

	HashMapUtils::CompactValuePrecision cvPrecision = HashMapUtils::CompactValuePrecision::kUInt8;
	size_t cvSize = sizeof(size_t);
	cvPrecision = HashMapUtils::CompactValuePrecision::kUInt64;

	size_t valueMainPosPlusOnePos = hashesPos + sizeof(Hash_t) * size;
	valueMainPosPlusOnePos += cvSize - 1;
	valueMainPosPlusOnePos -= valueMainPosPlusOnePos % cvSize;


	size_t nextPlusOnePos = valueMainPosPlusOnePos + cvSize * size;
	nextPlusOnePos += cvSize - 1;
	nextPlusOnePos -= nextPlusOnePos % cvSize;

	size_t bufferSize = nextPlusOnePos + cvSize * size;

	void *newBuffer = m_alloc.Alloc(bufferSize);
	if (!newBuffer)
		return rkc::ResultCodes::kOutOfMemory;

	const size_t oldCapacity = this->m_capacity;
	HashMapUtils::CompactValuePrecision oldCVPrecision = this->m_cvPrecision;
	void *oldBuffer = m_buffer;
	TKey *oldKeys = m_keys;
	TValue *oldValues = m_values;
	const Hash_t *oldHashes = m_hashes;
	const size_t *oldValueMainPosPlusOne = m_valueMainPosPlusOne;
	const size_t *oldNextPlusOne = m_nextPlusOne;

	m_buffer = newBuffer;
	m_capacity = size;
	m_cvPrecision = cvPrecision;
	m_keys = reinterpret_cast<TKey*>(reinterpret_cast<uint8_t*>(newBuffer) + keysPos);
	m_values = reinterpret_cast<TValue*>(reinterpret_cast<uint8_t*>(newBuffer) + valuesPos);
	m_hashes = reinterpret_cast<Hash_t*>(reinterpret_cast<uint8_t*>(newBuffer) + hashesPos);
	m_valueMainPosPlusOne = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(newBuffer) + valueMainPosPlusOnePos);
	m_nextPlusOne = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(newBuffer) + nextPlusOnePos);
	m_used = 0;
	m_freeSlotScan = 0;

	memset(m_valueMainPosPlusOne, 0, bufferSize - valueMainPosPlusOnePos);

	for (size_t i = 0; i < oldCapacity; i++)
	{
		if (HashMapUtils::GetCompactValue(oldValueMainPosPlusOne, oldCVPrecision, i) != 0)
		{
			const size_t mainPos = HashMapUtils::GetMainPosition(oldHashes[i], size);
			const size_t mpValueMainPosPlusOne = HashMapUtils::GetCompactValue(m_valueMainPosPlusOne, cvPrecision, mainPos);
			Result insertResult(InsertNew(rkci::Move(oldKeys[i]), rkci::Move(oldValues[i]), oldHashes[i], mainPos, mpValueMainPosPlusOne, false));
			if (!insertResult.IsOK())
			{
				for (size_t cleanupIndex = 0; cleanupIndex < oldCapacity; cleanupIndex++)
				{
					if (HashMapUtils::GetCompactValue(oldValueMainPosPlusOne, oldCVPrecision, cleanupIndex) != 0)
					{
						oldKeys[cleanupIndex].~TKey();
						oldValues[cleanupIndex].~TValue();
					}
				}
				m_alloc.Release(oldBuffer);
				return insertResult;
			}
			else
				insertResult.Handle();
		}
	}

	for (size_t i = 0; i < oldCapacity; i++)
	{
		if (HashMapUtils::GetCompactValue(oldValueMainPosPlusOne, oldCVPrecision, i) != 0)
		{
			oldKeys[i].~TKey();
			oldValues[i].~TValue();
		}
	}

	m_alloc.Release(oldBuffer);

	return Result::Ok();
}

template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::AutoRehash()
{
	size_t preferredSize = 8;
	while (preferredSize / 2 <= m_used)
		preferredSize *= 2;

	return Rehash(preferredSize);
}


template<class TKey, class TValue>
rkci::Result rkci::HashMap<TKey, TValue>::InsertNew(TKey &&key, TValue &&value, Hash_t keyHash, size_t keyMainPosition, size_t mpValueMPPlusOne, bool mayResize)
{
	// Try to find a free position
	if (mpValueMPPlusOne == 0)
	{
		// Main position is free
		new (&m_keys[keyMainPosition]) TKey(rkci::Move(key));
		new (&m_values[keyMainPosition]) TValue(rkci::Move(value));
		m_hashes[keyMainPosition] = keyHash;
		HashMapUtils::SetCompactValue(m_valueMainPosPlusOne, m_cvPrecision, keyMainPosition, keyMainPosition + 1);
		m_used++;

		return Result::Ok();
	}

	while (m_freeSlotScan < m_capacity)
	{
		if (HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, m_freeSlotScan) == 0)
			break;
	}

	// Couldn't find a free spot, rehash and try again
	if (m_freeSlotScan == m_capacity)
	{
		if (!mayResize)
			return rkc::ResultCodes::kInternalError;

		RKC_CHECK(AutoRehash());

		const size_t keyMainPosition = HashMapUtils::GetMainPosition(keyHash, m_capacity);
		const size_t mpValueMPPlusOne = HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, keyMainPosition);

		return InsertNew(rkci::Move(key), rkci::Move(value), keyHash, keyMainPosition, mpValueMPPlusOne, false);
	}

	const size_t freeSlotIndex = m_freeSlotScan++;

	if (mpValueMPPlusOne - 1 != keyMainPosition)
	{
		// Colliding node is not in main position, move it into the free slot and chain it into this.
		size_t otherIndex = mpValueMPPlusOne - 1;
		size_t nextIndexPlusOne = HashMapUtils::GetCompactValue(this->m_nextPlusOne, m_cvPrecision, otherIndex);
		while (nextIndexPlusOne != keyMainPosition + 1)
		{
			RKC_ASSERT(nextIndexPlusOne > 0);
			otherIndex = nextIndexPlusOne - 1;
			nextIndexPlusOne = HashMapUtils::GetCompactValue(this->m_nextPlusOne, m_cvPrecision, otherIndex);
		}

		// precedingIndex -> freeSlotIndex -> keyMainPosition -> 0

		const size_t precedingIndex = otherIndex;
		HashMapUtils::SetCompactValue(m_nextPlusOne, m_cvPrecision, precedingIndex, freeSlotIndex + 1);
		HashMapUtils::SetCompactValue(m_nextPlusOne, m_cvPrecision, keyMainPosition, 0);

		new (&m_keys[freeSlotIndex]) TKey(rkci::Move(m_keys[keyMainPosition]));
		new (&m_values[freeSlotIndex]) TValue(rkci::Move(m_values[keyMainPosition]));
		m_hashes[freeSlotIndex] = m_hashes[keyMainPosition];
		HashMapUtils::SetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, freeSlotIndex, mpValueMPPlusOne);

		m_keys[keyMainPosition].~TKey();
		m_values[keyMainPosition].~TValue();
		new (&m_keys[keyMainPosition]) TKey(rkci::Move(key));
		new (&m_values[keyMainPosition]) TValue(rkci::Move(value));
		m_hashes[keyMainPosition] = keyHash;

		HashMapUtils::SetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, keyMainPosition, keyMainPosition + 1);
	}
	else
	{
		// Colliding node is in its main position
		// Before : keyMainPosition -> ...
		// After: keyMainPosition -> freeSlotIndex -> ...

		const size_t mpNextPlusOne = HashMapUtils::GetCompactValue(m_nextPlusOne, m_cvPrecision, keyMainPosition);
		HashMapUtils::SetCompactValue(m_nextPlusOne, m_cvPrecision, freeSlotIndex, mpNextPlusOne);
		HashMapUtils::SetCompactValue(m_nextPlusOne, m_cvPrecision, keyMainPosition, freeSlotIndex + 1);

		new (&m_keys[freeSlotIndex]) TKey(rkci::Move(key));
		new (&m_values[freeSlotIndex]) TValue(rkci::Move(value));
		m_hashes[freeSlotIndex] = keyHash;
		HashMapUtils::SetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, freeSlotIndex, keyMainPosition + 1);
	}

	m_used++;
	return Result::Ok();
}

template<class TKey, class TValue>
template<class TKeyCandidate>
size_t rkci::HashMap<TKey, TValue>::FindIndex(const TKeyCandidate &keyCandidate)
{
	const Hash_t keyHash = Hasher<TKey>::Compute(keyCandidate);
	const size_t keyMainPosition = HashMapUtils::GetMainPosition(keyHash, m_capacity);
	const size_t mpValueMPPlusOne = HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, keyMainPosition);

	// Find existing key
	{
		size_t index = keyMainPosition;
		size_t valueMPPlusOne = mpValueMPPlusOne;
		for (;;)
		{
			if (valueMPPlusOne != 0 && Comparer<TKey>::StrictlyEqual(m_keys[index], keyCandidate))
				return index;

			const size_t nextIndexPlusOne = HashMapUtils::GetCompactValue(m_nextPlusOne, m_cvPrecision, index);
			if (nextIndexPlusOne == 0)
				return m_capacity;
		}
	}
}

template<class TKey, class TValue>
template<class TCandidateKey>
rkci::Optional<size_t> rkci::HashMap<TKey, TValue>::FindKey(const TCandidateKey &key) const
{
	if (m_used == 0)
		return rkci::Optional<size_t>();

	const Hash_t keyHash = Hasher<TKey>::Compute(key);
	const size_t keyMainPosition = HashMapUtils::GetMainPosition(keyHash);

	size_t scanPosition = keyMainPosition;
	for (;;)
	{
		const size_t mainPosPlusOne = HashMapUtils::GetCompactValue(this->m_valueMainPosPlusOne, m_cvPrecision, scanPosition);

		if (mainPosPlusOne != 0)
		{
			if (Comparer<TKey>::StrictlyEqual(m_keys[scanPosition], key))
				return scanPosition;
		}

		const size_t nextPlusOne = HashMapUtils::GetCompactValue(this->m_nextPlusOne, m_cvPrecision, scanPosition);
		if (nextPlusOne == 0)
			return rkci::Optional<size_t>();

		scanPosition = nextPlusOne - 1;
	}
}

inline size_t rkci::HashMapUtils::GetCompactValue(const size_t *items, CompactValuePrecision cvPrecision, size_t index)
{
	return items[index];
}

inline void rkci::HashMapUtils::SetCompactValue(size_t *items, CompactValuePrecision cvPrecision, size_t index, size_t value)
{
	items[index] = value;
}

inline size_t rkci::HashMapUtils::GetMainPosition(Hash_t hash, size_t count)
{
	return static_cast<size_t>(hash % count);
}
