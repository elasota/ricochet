#include "NumUtils.h"
#include "Result.h"
#include "BigUFloat.h"
#include "BigUDecFloatProto.h"
#include "BigUBinFloatProto.h"
#include "FloatSpec.h"
#include "MoveOrCopy.h"

rkci::ResultRV<rkci::BigUBinFloat_t> rkci::NumUtils::RoundToFloatSpec(const MoveOrCopy<BigUBinFloat_t> &fMOC, const FloatSpec &floatSpec)
{
	const BigUBinFloat_t &f = fMOC.Get();

	if (f.IsZero())
		return BigUBinFloat_t();

	const int32_t highBit = static_cast<int32_t>(f.GetNumDigits()) + f.GetLowPlace() - 1;

	const int32_t encodedExponent = highBit + floatSpec.GetExponentOfOne();
	int32_t lastBit = highBit - static_cast<int32_t>(floatSpec.GetMantissaBits());
	if (floatSpec.SupportsDenormals() && encodedExponent <= 0)
	{
		lastBit = 1 - static_cast<int32_t>(floatSpec.GetExponentOfOne()) - static_cast<int32_t>(floatSpec.GetMantissaBits());

		const int32_t lowestPossibleBit = lastBit - 1;
		if (highBit < lowestPossibleBit)
			return BigUBinFloat_t();
	}

	const int32_t roundingBitPos = lastBit - 1;
	const int32_t roundingBitPosRelativeToLowBit = roundingBitPos - f.GetLowPlace();

	bool roundUp = false;
	if (roundingBitPosRelativeToLowBit == 0)
	{
		// Rounding bit at position 0, so this is a tie
		roundUp = ((f.GetFragment(0) & 2) != 0);
	}
	else if (roundingBitPosRelativeToLowBit < 0)
	{
		// Rounding bit is below 0, so not significant AND no rounding to be done.
		RKC_CHECK_RV(BigUBinFloat_t, result, fMOC.Produce());
		return result;
	}
	else //if (roundingBitPosRelativeToLowBit > 0)
	{
		roundUp = true;
	}


	BigUBinFloat_t::FragmentVector_t newFragments(f.GetAllocator());

	const bool needsCarryBitFragment = ((f.GetNumDigits() % BigUBinFloat_t::kDigitsPerFragment) == 0);
	const size_t numFragments = f.GetNumFragments();

	RKC_CHECK(newFragments.Resize(numFragments + (needsCarryBitFragment ? 1 : 0)));

	ArraySliceView<BigUBinFloat_t::Fragment_t> newFragmentsSlice = newFragments.Slice();

	const size_t lastBitPosRelativeToLowBit = static_cast<size_t>(roundingBitPosRelativeToLowBit + 1);

	const size_t lastBitFragment = lastBitPosRelativeToLowBit / BigUBinFloat_t::kDigitsPerFragment;
	const size_t lastBitOffsetInFragment = lastBitPosRelativeToLowBit % BigUBinFloat_t::kDigitsPerFragment;

	for (size_t i = lastBitFragment; i < numFragments; i++)
		newFragmentsSlice[i] = f.GetFragment(i);

	BigUBinFloat_t::Fragment_t roundedOffBits = newFragmentsSlice[lastBitFragment] % BigUBinFloat_t::Properties_t::GetFragmentPower(lastBitOffsetInFragment);
	newFragmentsSlice[lastBitFragment] -= roundedOffBits;

	if (roundUp)
	{
		BigUBinFloat_t::FragmentWithCarry_t initialAdd = static_cast<BigUBinFloat_t::FragmentWithCarry_t>(newFragmentsSlice[lastBitFragment]) + (static_cast<BigUBinFloat_t::FragmentWithCarry_t>(1) << lastBitOffsetInFragment);
		if (initialAdd < BigUBinFloat_t::kFragmentModulo)
			newFragmentsSlice[lastBitFragment] = static_cast<BigUBinFloat_t::Fragment_t>(initialAdd);
		else
		{
			newFragments[lastBitFragment] = 0;	// All bits after the round bit are guaranteed to be zero, so if this carried, then none of the remaining bits are zero
			size_t carryFragment = lastBitFragment + 1;
			for (;;)
			{
				if (newFragments[carryFragment] == BigUBinFloat_t::kFragmentModulo - 1)
				{
					newFragments[carryFragment] = 0;
					carryFragment++;
				}
				else
				{
					newFragments[carryFragment]++;
					break;
				}
			}
		}
	}

	uint32_t lowBitsRemoved = 0;
	uint32_t significantDigits = 0;
	RKC_CHECK(BigUBinFloat_t::NormalizeFragments(newFragments, lowBitsRemoved, significantDigits));

	const int32_t newLowPos = f.GetLowPlace() + static_cast<int32_t>(lowBitsRemoved);

	if (newLowPos < BigUBinFloat_t::kMinLowPlace || newLowPos > BigUBinFloat_t::kMaxLowPlace)
		return rkc::ResultCodes::kIntegerOverflow;

	return BigUBinFloat_t(newLowPos, significantDigits, rkci::Move(newFragments));
}

