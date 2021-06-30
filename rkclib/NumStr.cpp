#include "NumStr.h"
#include "ArraySliceView.h"
#include "BigUDecFloatProto.h"
#include "BigUFloat.h"
#include "Unicode.h"
#include "CharCodes.h"

#include <cstring>

rkci::NumStr::NumStr(IAllocator &alloc)
	: m_alloc(alloc)
{
}

rkci::ResultRV<rkci::BigUDecFloat_t> rkci::NumStr::DecimalUTF8ToDecInt(const ArraySliceView<const uint8_t> &utf8Str, uint32_t &outNumTrailingZeroes) const
{
	rkci::BigUDecFloat_t result;

	const size_t strLength = utf8Str.Count();

	rkci::BigUDecFloat_t::Fragment_t fragment = 0;
	size_t fragmentLength = 0;
	int32_t fragmentPow10Shift = 0;
	int32_t resultPow10Shift = 0;

	for (uint8_t codePoint : utf8Str)
	{
		if (codePoint < CharCodes::kDigit0 || codePoint > CharCodes::kDigit9)
			return rkc::ResultCodes::kMalformedNumber;

		resultPow10Shift++;
		if (codePoint == CharCodes::kDigit0)
		{
			if (fragmentLength > 0)
				fragmentPow10Shift++;
		}
		else
		{
			fragmentPow10Shift++;
			if (fragmentLength + static_cast<size_t>(fragmentPow10Shift) > rkci::BigUDecFloat_t::kDigitsPerFragment)
			{
				rkci::BigUDecFloat_t fragmentBigUFloat = rkci::BigUDecFloat_t(fragment, m_alloc);
				RKC_CHECK(result.ShiftInPlace(resultPow10Shift));
				RKC_CHECK(fragmentBigUFloat.ShiftInPlace(fragmentPow10Shift));
				RKC_CHECK(result.AddInPlace(fragmentBigUFloat));

				resultPow10Shift = 0;
				fragmentPow10Shift = 1;
				fragment = 0;
				fragmentLength = 0;
			}

			fragment = fragment * rkci::BigUDecFloatProperties::GetFragmentPower(fragmentPow10Shift) + (static_cast<rkci::BigUDecFloat_t::Fragment_t>(codePoint - CharCodes::kDigit0));
			fragmentLength += fragmentPow10Shift;
			fragmentPow10Shift = 0;
		}
	}

	RKC_CHECK(result.ShiftInPlace(resultPow10Shift));

	if (fragmentLength > 0)
	{
		rkci::BigUDecFloat_t fragmentBigUFloat = rkci::BigUDecFloat_t(fragment, m_alloc);
		RKC_CHECK(fragmentBigUFloat.ShiftInPlace(fragmentPow10Shift));
		RKC_CHECK(result.AddInPlace(fragmentBigUFloat));
	}

	outNumTrailingZeroes = static_cast<uint32_t>(fragmentPow10Shift);

	return result;
}

rkci::ResultRV<rkci::BigUDecFloat_t> rkci::NumStr::DecimalUTF8ToDecFloat(const ArraySliceView<const uint8_t> &utf8Str, uint32_t &outNumTrailingZeroes) const
{
	bool hasE = false;
	bool hasDot = false;
	bool expHasSign = false;
	bool expIsNegative = false;
	size_t eLoc = 0;
	size_t expLoc = 0;
	size_t dotLoc = 0;
	size_t periodLoc = 0;

	const size_t strLength = utf8Str.Count();

	for (size_t i = 0; i < strLength; i++)
	{
		const uint8_t codePoint = utf8Str[i];

		if (codePoint == CharCodes::kPeriod)
		{
			if (hasDot || hasE)
				return rkc::ResultCodes::kMalformedNumber;
			else
			{
				hasDot = true;
				dotLoc = i;
			}
		}
		else if (codePoint == CharCodes::kUppercaseE || codePoint == CharCodes::kLowercaseE)
		{
			if (hasE || i == strLength - 1)
				return rkc::ResultCodes::kMalformedNumber;
			else
			{
				eLoc = i;
				const uint8_t nextCodePoint = utf8Str[i + 1];
				if (nextCodePoint == '-')
				{
					i++;
					expIsNegative = true;

					if (i == strLength - 1)
						return rkc::ResultCodes::kMalformedNumber;
				}
				else if (nextCodePoint == '+')
				{
					i++;
					expIsNegative = true;

					if (i == strLength - 1)
						return rkc::ResultCodes::kMalformedNumber;
				}
				expLoc = i;

				hasE = true;
			}
		}
		else if (codePoint < CharCodes::kDigit0 || codePoint > CharCodes::kDigit9)
			return rkc::ResultCodes::kMalformedNumber;
	}

	size_t integralDigits = 0;
	size_t fractionalDigits = 0;
	size_t expDigits = 0;

	if (hasDot)
	{
		integralDigits = dotLoc;
		if (hasE)
		{
			fractionalDigits = eLoc - dotLoc - 1;
			expDigits = strLength - expLoc;
		}
		else
			fractionalDigits = strLength - dotLoc - 1;
	}
	else
	{
		if (hasE)
		{
			integralDigits = eLoc - dotLoc - 1;
			expDigits = strLength - expLoc;
		}
		else
			integralDigits = strLength;
	}

	int32_t exponentOffset = 0;

	if (hasE)
	{
		return rkc::ResultCodes::kNotYetImplemented;
	}

	uint32_t numTrailingZeroes = 0;
	RKC_CHECK_RV(rkci::BigUDecFloat_t, result, DecimalUTF8ToDecInt(utf8Str.Subrange(0, integralDigits), numTrailingZeroes));
	if (hasDot)
	{
		uint32_t fracTrailingZeroes = 0;
		RKC_CHECK_RV(rkci::BigUDecFloat_t, fractionalPart, DecimalUTF8ToDecInt(utf8Str.Subrange(dotLoc + 1, fractionalDigits), fracTrailingZeroes));

		if (fractionalPart.IsZero())
			numTrailingZeroes += fracTrailingZeroes;
		else
		{
			RKC_CHECK(fractionalPart.ShiftInPlace(-static_cast<int32_t>(fractionalDigits)));
			RKC_CHECK(result.AddInPlace(fractionalPart));
			numTrailingZeroes = fracTrailingZeroes;
		}
	}

	outNumTrailingZeroes = numTrailingZeroes;

	return result;
}
