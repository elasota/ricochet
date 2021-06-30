#pragma once

#include "CoreDefs.h"
#include "BigUBinFloatProto.h"
#include "BigUDecFloatProto.h"

namespace rkci
{
	class FloatSpec;
	template<class T> class ResultRV;
	template<class T> class MoveOrCopy;

	struct DecBin
	{
		static ResultRV<BigUDecFloat_t> BinToDec(const MoveOrCopy<BigUBinFloat_t> &bin);
		static ResultRV<BigUDecFloat_t> BinToDecWithFloatSpec(const MoveOrCopy<BigUBinFloat_t> &bin, const FloatSpec &floatSpec);
		static ResultRV<BigUBinFloat_t> DecToBin(const MoveOrCopy<BigUDecFloat_t> &dec, const FloatSpec &floatSpec, uint32_t numSignificantTrailingZeroes);

		// Returns a binary float from an integral decimal float
		static ResultRV<BigUBinFloat_t> DecToBinInteger(const BigUDecFloat_t &dec);
		// Returns a binary float from an inexact decimal float that can't be rounded to a power of two
		static ResultRV<BigUBinFloat_t> DecToBinNonExact(const BigUDecFloat_t &dec, const FloatSpec &floatSpec, uint32_t numSignificantTrailingZeroes);
	};
}
