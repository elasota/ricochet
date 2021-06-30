#include "DecBin.h"
#include "BigUDecFloatProto.h"
#include "BigUBinFloatProto.h"
#include "BigUFloat.h"
#include "Result.h"
#include "NumUtils.h"
#include "FloatSpec.h"
#include "MoveOrCopy.h"


rkci::ResultRV<rkci::BigUDecFloat_t> rkci::DecBin::BinToDecWithFloatSpec(const MoveOrCopy<BigUBinFloat_t> &binMOC, const FloatSpec &floatSpec)
{
	const BigUBinFloat_t &bin = binMOC.Get();

	if (bin.IsZero())
	{
		binMOC.Consume();
		return BigUDecFloat_t();
	}

	const int32_t highBitPos = bin.GetLowPlace() + static_cast<int32_t>(bin.GetNumDigits()) - 1;
	const int32_t codedExponent = highBitPos + floatSpec.GetExponentOfOne();

	int32_t lowestPossibleBitPos = highBitPos - static_cast<int32_t>(floatSpec.GetMantissaBits());
	if (codedExponent <= 0 && floatSpec.SupportsDenormals())
		lowestPossibleBitPos = 1 - static_cast<int32_t>(floatSpec.GetMantissaBits()) - static_cast<int32_t>(floatSpec.GetExponentOfOne());

	if (bin.GetLowPlace() < lowestPossibleBitPos)
	{
		RKC_CHECK_RV(BigUBinFloat_t, rounded, NumUtils::RoundToFloatSpec(binMOC, floatSpec));
		return BinToDecWithFloatSpec(rkci::Move(rounded), floatSpec);
	}

	BigUBinFloat_t nextAboveStep;
	BigUBinFloat_t nextBelowStep;

	{
		nextAboveStep = BigUBinFloat_t(1, *bin.GetAllocator());
		RKC_CHECK(nextAboveStep.ShiftInPlace(lowestPossibleBitPos));
	}

	{
		nextBelowStep = BigUBinFloat_t(1, *bin.GetAllocator());
		if (bin.GetNumDigits() == 1)
		{
			// Subtracting 1 will produce a lower value
			if (codedExponent > 1)
			{
				// Lower bit is 1 below, since this will drop the exponent by 1
				RKC_CHECK(nextBelowStep.ShiftInPlace(lowestPossibleBitPos - 1));
			}
			else if (codedExponent == 1)
			{
				if (floatSpec.SupportsDenormals())
				{
					// Will drop into subnormal range
					RKC_CHECK(nextBelowStep.ShiftInPlace(lowestPossibleBitPos));
				}
				else
				{
					// Will drop down 1 exponent
					RKC_CHECK(nextBelowStep.ShiftInPlace(lowestPossibleBitPos - 1));
				}
			}
			else //if (codedExponent <= 0)
			{
				if (floatSpec.SupportsDenormals())
				{
					RKC_CHECK(nextBelowStep.ShiftInPlace(lowestPossibleBitPos));
				}
				else
				{
					RKC_CHECK_RV(BigUBinFloat_t, clone, bin.Clone());
					nextBelowStep = rkci::Move(clone);
				}
			}
		}
		else
		{
			RKC_CHECK(nextBelowStep.ShiftInPlace(lowestPossibleBitPos));
		}
	}

	// Halve steps to create midpoint increments
	RKC_CHECK(nextAboveStep.ShiftInPlace(-1));
	RKC_CHECK(nextBelowStep.ShiftInPlace(-1));

	RKC_CHECK_RV(BigUBinFloat_t, upperBounds, bin.Clone());
	RKC_CHECK_RV(BigUBinFloat_t, lowerBounds, bin.Clone());
	binMOC.Consume();

	RKC_CHECK(upperBounds.AddInPlace(nextAboveStep));
	nextAboveStep = BigUBinFloat_t();

	RKC_CHECK(lowerBounds.SubtractInPlace(nextBelowStep));
	nextBelowStep = BigUBinFloat_t();

	RKC_CHECK_RV(BigUDecFloat_t, upperBoundsDec, DecBin::BinToDec(upperBounds));
	upperBounds = BigUBinFloat_t();

	RKC_CHECK_RV(BigUDecFloat_t, lowerBoundsDec, DecBin::BinToDec(lowerBounds));
	lowerBounds = BigUBinFloat_t();

	// The true value falls exclusively between upperBoundsDec and lowerBoundsDec
	const int32_t upperHighDigitExclusive = upperBoundsDec.GetLowPlace() + static_cast<int32_t>(upperBoundsDec.GetNumDigits());
	const int32_t lowerHighDigitExclusive = lowerBoundsDec.GetLowPlace() + static_cast<int32_t>(lowerBoundsDec.GetNumDigits());

	if (upperHighDigitExclusive > lowerHighDigitExclusive)
	{
		// Lower doesn't even have this digit at all.
		const int32_t lowestSignificantDigit = upperHighDigitExclusive - 1;
		const uint32_t highDigitPowerInFragment = (upperBoundsDec.GetNumDigits() - 1) % BigUDecFloat_t::kDigitsPerFragment;
		const BigUDecFloat_t::Fragment_t highDigit = upperBoundsDec.GetFragment(upperBoundsDec.GetNumFragments() - 1) / BigUDecFloat_t::Properties_t::GetFragmentPower(highDigitPowerInFragment);

		const int32_t highDigitPlace = upperHighDigitExclusive - 1;

		BigUDecFloat_t result(highDigit, *bin.GetAllocator());
		RKC_CHECK(result.ShiftInPlace(highDigitPlace));

		return result;
	}
	else
	{
		RKC_ASSERT(lowerHighDigitExclusive == upperHighDigitExclusive);	// This shouldn't be possible

		uint32_t upperDistanceFromLowDigit = static_cast<uint32_t>(upperHighDigitExclusive - upperBoundsDec.GetLowPlace());
		uint32_t lowerDistanceFromLowDigit = static_cast<uint32_t>(lowerHighDigitExclusive - lowerBoundsDec.GetLowPlace());

		RKC_ASSERT(upperDistanceFromLowDigit / BigUDecFloat_t::kDigitsPerFragment == upperBoundsDec.GetNumFragments() - 1);
		RKC_ASSERT(lowerDistanceFromLowDigit / BigUDecFloat_t::kDigitsPerFragment == lowerBoundsDec.GetNumFragments() - 1);

		BigUDecFloat_t::Fragment_t upperRemainder = upperBoundsDec.GetFragment(upperBoundsDec.GetNumFragments() - 1);
		BigUDecFloat_t::Fragment_t lowerRemainder = lowerBoundsDec.GetFragment(lowerBoundsDec.GetNumFragments() - 1);

		BigUDecFloat_t::FragmentVector_t reconstructedFrags(bin.GetAllocator());

		uint32_t fragTopPos = 0;

		for (;;)
		{
			// This shouldn't be possible because the halfway point reduction should be at a lower place than the last significant digit
			if (upperDistanceFromLowDigit == 0 || lowerDistanceFromLowDigit == 0)
			{
				RKC_ASSERT(false);
				return rkc::ResultCodes::kInternalError;
			}

			upperDistanceFromLowDigit--;
			lowerDistanceFromLowDigit--;

			const uint32_t upperFragmentPlace = upperDistanceFromLowDigit % BigUDecFloat_t::kDigitsPerFragment;
			const uint32_t lowerFragmentPlace = lowerDistanceFromLowDigit % BigUDecFloat_t::kDigitsPerFragment;

			const BigUDecFloat_t::Fragment_t upperHigh = upperRemainder / BigUDecFloat_t::Properties_t::GetFragmentPower(upperFragmentPlace);
			const BigUDecFloat_t::Fragment_t upperLow = upperRemainder % BigUDecFloat_t::Properties_t::GetFragmentPower(upperFragmentPlace);

			const BigUDecFloat_t::Fragment_t lowerHigh = lowerRemainder / BigUDecFloat_t::Properties_t::GetFragmentPower(lowerFragmentPlace);
			const BigUDecFloat_t::Fragment_t lowerLow = lowerRemainder % BigUDecFloat_t::Properties_t::GetFragmentPower(lowerFragmentPlace);

			upperRemainder = upperLow;
			lowerRemainder = lowerLow;

			if (fragTopPos == 0)
			{
				RKC_CHECK(reconstructedFrags.Append(0));
				fragTopPos = BigUDecFloat_t::kDigitsPerFragment;
			}

			fragTopPos--;
			reconstructedFrags[reconstructedFrags.Count() - 1] += upperHigh * BigUDecFloat_t::Properties_t::GetFragmentPower(fragTopPos);

			// Found the mismatched digit
			if (upperHigh != lowerHigh)
				break;

			if (upperFragmentPlace == 0)
			{
				RKC_ASSERT(upperDistanceFromLowDigit > 0);
				upperRemainder = upperBoundsDec.GetFragment(upperDistanceFromLowDigit / BigUDecFloat_t::kDigitsPerFragment - 1);
			}

			if (lowerFragmentPlace == 0)
			{
				RKC_ASSERT(lowerDistanceFromLowDigit > 0);
				lowerRemainder = lowerBoundsDec.GetFragment(lowerDistanceFromLowDigit / BigUDecFloat_t::kDigitsPerFragment - 1);
			}
		}

		// Reverse fragment order
		const size_t halfReconSize = reconstructedFrags.Count() / 2;
		for (size_t i = 0; i < halfReconSize; i++)
			std::swap(reconstructedFrags[i], reconstructedFrags[reconstructedFrags.Count() - 1 - i]);

		// Normalize
		uint32_t removedLowDigits = 0;
		uint32_t significantDigits = 0;
		RKC_CHECK(BigUDecFloat_t::NormalizeFragments(reconstructedFrags, removedLowDigits, significantDigits));

		const int32_t newLowPlace = upperBoundsDec.GetLowPlace() + static_cast<int32_t>(upperDistanceFromLowDigit) - static_cast<int32_t>(fragTopPos) + static_cast<int32_t>(removedLowDigits);
		if (newLowPlace < BigUDecFloat_t::kMinLowPlace || newLowPlace > BigUDecFloat_t::kMaxLowPlace || significantDigits > BigUDecFloat_t::kMaxDigits)
			return rkc::ResultCodes::kIntegerOverflow;

		return BigUDecFloat_t(newLowPlace, significantDigits, rkci::Move(reconstructedFrags));
	}
}

rkci::ResultRV<rkci::BigUDecFloat_t> rkci::DecBin::BinToDec(const MoveOrCopy<BigUBinFloat_t> &binMOC)
{
	const BigUBinFloat_t &bin = binMOC.Get();
	if (bin.IsZero())
		return BigUDecFloat_t();

	const unsigned int kBitsPerSlice = 16;
	RKC_STATIC_ASSERT(BigUBinFloat_t::kDigitsPerFragment % kBitsPerSlice == 0);
	RKC_STATIC_ASSERT((static_cast<uintmax_t>(1) << kBitsPerSlice) - 1 < BigUDecFloat_t::kFragmentModulo);

	const BigUDecFloat_t sliceRaise(static_cast<BigUDecFloat_t::Fragment_t>(1) << kBitsPerSlice, *bin.GetAllocator());

	const int32_t lowPlace = bin.GetLowPlace();

	BigUDecFloat_t currentSliceMultiplier;
	if (lowPlace < 0)
	{
		RKC_CHECK_RV(BigUDecFloat_t, multiplier , NumUtils::PositivePow(BigUDecFloat_t(5, *bin.GetAllocator()), static_cast<uint32_t>(-lowPlace)));
		RKC_CHECK(multiplier.ShiftInPlace(lowPlace));
		currentSliceMultiplier = rkci::Move(multiplier);
	}
	else if (lowPlace > 0)
	{
		RKC_CHECK_RV(BigUDecFloat_t, multiplier, NumUtils::PositivePow(BigUDecFloat_t(2, *bin.GetAllocator()), static_cast<uint32_t>(lowPlace)));
		currentSliceMultiplier = rkci::Move(multiplier);
	}
	else //if (lowPlace == 0)
		currentSliceMultiplier = BigUDecFloat_t(1, *bin.GetAllocator());

	const BigUBinFloat_t::Fragment_t kSliceLowMask = (((static_cast<BigUBinFloat_t::Fragment_t>(1) << (kBitsPerSlice - 1)) - 1) << 1) + 1;

	BigUDecFloat_t result;

	const uint32_t numSlices = (bin.GetNumDigits() + kBitsPerSlice - 1) / kBitsPerSlice;
	for (uint32_t sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
	{
		BigUBinFloat_t::Fragment_t fragment = bin.GetFragment(sliceIndex / kBitsPerSlice);
		const uint32_t subSliceIndex = sliceIndex % kBitsPerSlice;

		fragment = (fragment >> (subSliceIndex * kBitsPerSlice)) & kSliceLowMask;

		BigUDecFloat_t addend(static_cast<BigUDecFloat_t::Fragment_t>(fragment), *bin.GetAllocator());
		RKC_CHECK(addend.MultiplyInPlace(currentSliceMultiplier));

		RKC_CHECK(result.AddInPlace(addend));
		if (sliceIndex != numSlices - 1)
		{
			RKC_CHECK(currentSliceMultiplier.MultiplyInPlace(sliceRaise));
		}
	}

	return result;
}

rkci::ResultRV<rkci::BigUBinFloat_t> rkci::DecBin::DecToBin(const MoveOrCopy<BigUDecFloat_t> &decMOC, const FloatSpec &floatSpec, uint32_t numSignificantTrailingZeroes)
{
	const BigUDecFloat_t &dec = decMOC.Get();

	if (dec.IsZero())
		return rkci::BigUBinFloat_t();

	int32_t lowPlace = dec.GetLowPlace();
	if (dec.GetLowPlace() >= 0)
	{
		RKC_CHECK_RV(BigUBinFloat_t, binInt, DecToBinInteger(dec));
		RKC_CHECK_RV(BigUBinFloat_t, truncated, NumUtils::RoundToFloatSpec(rkci::Move(binInt), floatSpec));
		return truncated;
	}

	const BigUDecFloat_t *remPtr = &dec;

	uint32_t lowDigit = dec.GetFragment(0) % 10;
	if (lowDigit == 5)
	{
		// Could be an exact binary number
		BigUDecFloat_t two(2, *dec.GetAllocator());

		RKC_CHECK_RV(BigUDecFloat_t, twoToPowerOfTrailingDigits, NumUtils::PositivePow(two, -lowPlace));

		RKC_CHECK_RV(BigUDecFloat_t, decRaised, dec.Clone());
		RKC_CHECK(decRaised.MultiplyInPlace(twoToPowerOfTrailingDigits));
		if (decRaised.GetLowPlace() >= 0)
		{
			// Is an exact binary number
			twoToPowerOfTrailingDigits = BigUDecFloat_t();

			RKC_CHECK_RV(BigUBinFloat_t, binInt, DecToBinInteger(decRaised));
			decRaised = BigUDecFloat_t();

			RKC_CHECK(binInt.ShiftInPlace(lowPlace));
			RKC_CHECK_RV(BigUBinFloat_t, truncated, NumUtils::RoundToFloatSpec(binInt, floatSpec));

			return truncated;
		}
	}

	// Not an exact binary number
	return DecToBinNonExact(dec, floatSpec, numSignificantTrailingZeroes);
}


// Returns a binary float from an integral decimal float
rkci::ResultRV<rkci::BigUBinFloat_t> rkci::DecBin::DecToBinInteger(const BigUDecFloat_t &dec)
{
	if (dec.IsZero())
		return BigUBinFloat_t();

	RKC_ASSERT(dec.GetLowPlace() >= 0);

	IAllocator &alloc = *dec.GetAllocator();

	RKC_STATIC_ASSERT(BigUBinFloatProperties::kFragmentModulo >= BigUDecFloatProperties::kFragmentModulo);

	BigUBinFloat_t decFragmentModulo(BigUDecFloatProperties::kFragmentModulo, alloc);
	uint32_t numDecFragments = dec.GetNumFragments();

	BigUBinFloat_t fragmentRaise;

	BigUBinFloat_t result;

	for (uint32_t fragIndex = 0; fragIndex < numDecFragments; fragIndex++)
	{
		if (fragIndex == 0)
		{
		}
		else if (fragIndex == 1)
			fragmentRaise = BigUBinFloat_t(BigUDecFloatProperties::kFragmentModulo, alloc);
		else
		{
			RKC_CHECK(fragmentRaise.MultiplyInPlace(decFragmentModulo));
		}

		BigUBinFloat_t raised = BigUBinFloat_t(dec.GetFragment(fragIndex), alloc);

		if (fragIndex == 0)
			result = rkci::Move(raised);
		else
		{
			RKC_CHECK(raised.MultiplyInPlace(fragmentRaise));
			RKC_CHECK(result.AddInPlace(raised));
		}
	}

	return result;
}

// Returns a binary float from an inexact decimal float that can't be rounded to a power of two
rkci::ResultRV<rkci::BigUBinFloat_t> rkci::DecBin::DecToBinNonExact(const BigUDecFloat_t &dec, const FloatSpec &floatSpec, uint32_t numSignificantTrailingZeroes)
{
	RKC_ASSERT(!dec.IsZero());

	IAllocator &alloc = *dec.GetAllocator();

	int32_t topDecPositionExclusive = dec.GetLowPlace() + static_cast<int32_t>(dec.GetNumDigits());

	BigUDecFloat_t longDivideBitDec;

	int64_t topBinPositionExclusive = (static_cast<int64_t>(topDecPositionExclusive) * static_cast<int64_t>(108853) + 32767) >> 15;
	int32_t longDivideBitPosition = static_cast<int32_t>(topBinPositionExclusive - 1);

	if (longDivideBitPosition >= 0)
	{
		RKC_CHECK_RV(BigUDecFloat_t, raised, NumUtils::PositivePow(BigUDecFloat_t(2, alloc), static_cast<uint32_t>(longDivideBitPosition)));
		longDivideBitDec = rkci::Move(raised);
	}
	else
	{
		RKC_CHECK_RV(BigUDecFloat_t, raised, NumUtils::PositivePow(BigUDecFloat_t(5, alloc), static_cast<uint32_t>(-longDivideBitPosition)));
		RKC_CHECK(raised.ShiftInPlace(longDivideBitPosition));
		longDivideBitDec = rkci::Move(raised);
	}

	bool haveHighBit = false;
	int32_t highBitPos = 0;
	uint32_t numBitsResolved = 0;

	// There are 3 border cases here:
	// One is at the typical border of an exponent decrease.  i.e. assume 3 mantissa bits:
	// High: 1.000
	// Low:  0.1111
	// Mid:  0.11111
	// In this case, if we resolve 5 bits, 0.11110 is below the midpoint and 0.11111 is above the midpoint, because there is always a remainder in this function.
	//
	// The second is at the normal/denormal boundary.  i.e. assume 3 mantissa bit and 1.000 as the smallest normal number:
	// High: 1.000
	// Low:  0.111
	// Mid:  0.1111

	uint32_t targetBits = floatSpec.GetMantissaBits() + 3;

	RKC_CHECK_RV(BigUDecFloat_t, longDivideRemainder, dec.Clone());

	size_t targetFragments = (targetBits + BigUBinFloat_t::kDigitsPerFragment - 1) / BigUBinFloat_t::kDigitsPerFragment;

	BigUBinFloat_t::FragmentVector_t binFragments(dec.GetAllocator());
	RKC_CHECK(binFragments.Resize(targetFragments));

	ArraySliceView<BigUBinFloat_t::Fragment_t> fragSlice = binFragments.Slice();

	while (numBitsResolved < targetBits - 1)
	{
		bool isOneBit = (longDivideBitDec <= longDivideRemainder);
		if (isOneBit)
		{
			RKC_CHECK(longDivideRemainder.SubtractInPlace(longDivideBitDec));
			if (!haveHighBit)
			{
				haveHighBit = true;
				highBitPos = longDivideBitPosition;
			}

			// Insert bit
			size_t bitPos = static_cast<size_t>(static_cast<int32_t>(targetBits) - 1 + longDivideBitPosition - highBitPos);
			binFragments[bitPos / BigUBinFloat_t::kDigitsPerFragment] |= static_cast<BigUBinFloat_t::Fragment_t>(1) << (bitPos % BigUBinFloat_t::kDigitsPerFragment);
		}

		RKC_CHECK(longDivideBitDec.MultiplyInPlace(BigUDecFloat_t(5, *dec.GetAllocator())));
		RKC_CHECK(longDivideBitDec.ShiftInPlace(-1));
		longDivideBitPosition--;

		if (haveHighBit)
			numBitsResolved++;
	}

	// Insert tiebreak bit
	numBitsResolved++;
	binFragments[0] |= 1;

	const int32_t lowPlace = highBitPos - static_cast<int32_t>(numBitsResolved) + 1;
	if (lowPlace < BigUBinFloat_t::kMinLowPlace || lowPlace > BigUBinFloat_t::kMaxLowPlace)
		return rkc::ResultCodes::kIntegerOverflow;

	return NumUtils::RoundToFloatSpec(BigUBinFloat_t(lowPlace, numBitsResolved, rkci::Move(binFragments)), floatSpec);
}
