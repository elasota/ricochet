#pragma once

#include "CoreDefs.h"

namespace rkci
{
	template<class T> class BigUFloat;

	struct BigUBinFloatProperties
	{
		typedef uint32_t Fragment_t;
		typedef uint64_t FragmentWithCarry_t;
		typedef uint64_t DoubleFragment_t;

		static const Fragment_t kBase = 2;
		static const uint8_t kDigitsPerFragment = 32;
		static const FragmentWithCarry_t kFragmentModulo = static_cast<FragmentWithCarry_t>(1) << kDigitsPerFragment;
		static const unsigned int kNumStaticFragments = 2;

		static const size_t kMaxDigits = 0x10000;
		static const int32_t kMaxLowPlace = 0x10000;
		static const int32_t kMinLowPlace = -0x10000;

		static uint8_t CountTrailingZeroes(Fragment_t fragment);
		static Fragment_t GetFragmentPower(size_t index);
	};

	typedef BigUFloat<BigUBinFloatProperties> BigUBinFloat_t;
}

#include "BitUtils.h"

inline uint8_t rkci::BigUBinFloatProperties::CountTrailingZeroes(Fragment_t fragment)
{
	return BitUtils::FindLowestSetBit(fragment);
}

inline rkci::BigUBinFloatProperties::Fragment_t rkci::BigUBinFloatProperties::GetFragmentPower(size_t index)
{
	return static_cast<Fragment_t>(1) << index;
}
