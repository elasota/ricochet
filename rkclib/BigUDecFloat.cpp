#include "BigUDecFloatProto.h"



const rkci::BigUDecFloatProperties::Fragment_t rkci::BigUDecFloatProperties::kFragmentPowers[kDigitsPerFragment] =
{
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
};

uint8_t rkci::BigUDecFloatProperties::CountTrailingZeroes(Fragment_t fragment)
{
	RKC_ASSERT(fragment != 0);
	RKC_ASSERT(static_cast<FragmentWithCarry_t>(fragment < kFragmentModulo));

	uint8_t trailingZeroes = 0;
	uint8_t maxTrailingZeroes = kDigitsPerFragment - 1;

	while (maxTrailingZeroes > 0)
	{
		// Fast case: Odd numbers have no trailing zeroes
		if (fragment & 1)
			break;

		const uint8_t halfTrailingZeroes = (maxTrailingZeroes + 1) / 2;
		const Fragment_t modulo = GetFragmentPower(halfTrailingZeroes);

		const Fragment_t lowerHalf = fragment % modulo;
		if (lowerHalf == 0)
		{
			trailingZeroes += halfTrailingZeroes;
			maxTrailingZeroes -= halfTrailingZeroes;
			fragment = fragment / modulo;
		}
		else
		{
			// The most trailing zeroes that there could be in the lower half is 1 less than the number of digits cleaved off,
			// because if they were all zero, we would not be in this condition
			maxTrailingZeroes = halfTrailingZeroes - 1;
			fragment = lowerHalf;
		}
	}

	return trailingZeroes;
}
