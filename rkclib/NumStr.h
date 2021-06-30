#pragma once

#include "Vector.h"

#include "BigUDecFloatProto.h"

namespace rkci
{
	class Result;
	template<class T> class ResultRV;
	struct IAllocator;
	template<class... T> class Tuple;
	template<class T> class ArraySliceView;

	class NumStr
	{
	public:
		typedef rkci::Vector<uint8_t, 32> NumStrString_t;

		explicit NumStr(IAllocator &alloc);

		ResultRV<BigUDecFloat_t> DecimalUTF8ToDecInt(const ArraySliceView<const uint8_t> &utf8Str, uint32_t &outNumTrailingZeroes) const;
		ResultRV<BigUDecFloat_t> DecimalUTF8ToDecFloat(const ArraySliceView<const uint8_t> &utf8Str, uint32_t &outNumTrailingZeroes) const;

	private:
		IAllocator &m_alloc;
	};
}
