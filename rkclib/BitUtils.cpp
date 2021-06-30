#include "BitUtils.h"

#ifdef _MSC_VER
#include <intrin.h>

uint8_t rkci::BitUtils::FindLowestSetBit(uint32_t value)
{
	RKC_ASSERT(value != 0);
	unsigned long index = 0;
	if (_BitScanForward(&index, value))
		return static_cast<uint8_t>(index);
	return 0;
}

uint8_t rkci::BitUtils::FindLowestSetBit(uint64_t value)
{
	RKC_ASSERT(value != 0);
	unsigned long index = 0;
	if (_BitScanForward64(&index, value))
		return static_cast<uint8_t>(index);
	return 0;
}

uint8_t rkci::BitUtils::FindHighestSetBit(uint32_t value)
{
	RKC_ASSERT(value != 0);
	unsigned long index = 0;
	if (_BitScanReverse(&index, value))
		return static_cast<uint8_t>(index);
	return 0;
}

uint8_t rkci::BitUtils::FindHighestSetBit(uint64_t value)
{
	RKC_ASSERT(value != 0);
	unsigned long index = 0;
	if (_BitScanReverse64(&index, value))
		return static_cast<uint8_t>(index);
	return 0;
}

#endif
