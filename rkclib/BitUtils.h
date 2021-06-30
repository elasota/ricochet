#pragma once

#include "CoreDefs.h"

namespace rkci
{
	namespace BitUtils
	{
		uint8_t FindLowestSetBit(uint32_t value);
		uint8_t FindLowestSetBit(uint64_t value);
		uint8_t FindHighestSetBit(uint32_t value);
		uint8_t FindHighestSetBit(uint64_t value);
	}
}
