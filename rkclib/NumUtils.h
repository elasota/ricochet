#pragma once

#include "CoreDefs.h"
#include "BigUFloat.h"
#include "BigUBinFloatProto.h"
#include "BigUDecFloatProto.h"

namespace rkci
{
	template<class T> class ResultRV;
	template<class T> class MoveOrCopy;
	class FloatSpec;

	struct NumUtils
	{
		template<class T>
		static ResultRV<BigUFloat<T>> PositivePow(const BigUFloat<T> &f, uint32_t power);

		static ResultRV<BigUBinFloat_t> RoundToFloatSpec(const MoveOrCopy<BigUBinFloat_t> &f, const FloatSpec &floatSpec);
	};
}

template<class T>
rkci::ResultRV<rkci::BigUFloat<T>> rkci::NumUtils::PositivePow(const BigUFloat<T> &f, const uint32_t power)
{
	if (f.IsZero())
		return rkci::BigUFloat<T>();

	if (power == 0)
		return rkci::BigUFloat<T>(1, *f.GetAllocator());

	bool isInitialized = false;

	BigUFloat<T> result;
	BigUFloat<T> raised;

	uint32_t powerBits = power;

	for (int bit = 0; bit < 32; bit++)
	{
		if (bit == 0)
		{
			RKC_CHECK_RV(BigUDecFloat_t, cloned, f.Clone());
			raised = rkci::Move(cloned);
		}
		else
		{
			RKC_CHECK(raised.MultiplyInPlace(raised));
		}

		const uint32_t bitMask = static_cast<uint32_t>(1) << bit;
		if (power & bitMask)
		{
			if (!isInitialized)
			{
				RKC_CHECK_RV(BigUDecFloat_t, cloned, raised.Clone());
				result = rkci::Move(cloned);
				isInitialized = true;
			}
			else
			{
				RKC_CHECK(result.MultiplyInPlace(raised));
			}

			powerBits ^= bitMask;
			if (powerBits == 0)
				break;
		}
	}

	RKC_ASSERT(powerBits == 0);

	return result;
}
