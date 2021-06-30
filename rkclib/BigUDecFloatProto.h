#pragma once

#include "CoreDefs.h"

namespace rkci
{
	template<class T> class BigUFloat;

	struct BigUDecFloatProperties
	{
		typedef uint32_t Fragment_t;
		typedef uint32_t FragmentWithCarry_t;
		typedef uint64_t DoubleFragment_t;

		static const Fragment_t kBase = 10;
		static const uint8_t kDigitsPerFragment = 8;	// We could fit 9 digits, but using 8 digits allows strength reduction of a bunch of div/modulo ops
		static const FragmentWithCarry_t kFragmentModulo = 100000000;
		static const unsigned int kNumStaticFragments = 1;

		static const size_t kMaxDigits = 0x10000;
		static const int32_t kMaxLowPlace = 0x10000;
		static const int32_t kMinLowPlace = -0x10000;

		static const Fragment_t kFragmentPowers[kDigitsPerFragment];

		static uint8_t CountTrailingZeroes(Fragment_t fragment);
		static Fragment_t GetFragmentPower(size_t index);
	};

	typedef BigUFloat<BigUDecFloatProperties> BigUDecFloat_t;
}

inline rkci::BigUDecFloatProperties::Fragment_t rkci::BigUDecFloatProperties::GetFragmentPower(size_t index)
{
	RKC_ASSERT(index < kDigitsPerFragment);
	return kFragmentPowers[index];
}
