#pragma once

#include "BigUBinFloatProto.h"
#include "BigUFloat.h"

namespace rkci
{
	class BigSBinFloat
	{
	public:
		BigSBinFloat(BigUBinFloat_t &&bigUFloat, bool isNegative);
		BigSBinFloat(BigSBinFloat &&other);
		~BigSBinFloat();

		bool IsNegative() const;
		const BigUBinFloat_t &GetUFloat() const;

		ResultRV<BigSBinFloat> Clone() const;

	private:
		BigUBinFloat_t m_bigUFloat;
		bool m_isNegative;
	};
}

#include "Result.h"

inline rkci::BigSBinFloat::BigSBinFloat(BigUBinFloat_t &&bigUFloat, bool isNegative)
	: m_bigUFloat(rkci::Move(bigUFloat))
	, m_isNegative(isNegative)
{
}

inline rkci::BigSBinFloat::BigSBinFloat(BigSBinFloat &&other)
	: m_bigUFloat(rkci::Move(other.m_bigUFloat))
	, m_isNegative(other.m_isNegative)
{
}

inline rkci::BigSBinFloat::~BigSBinFloat()
{
}

inline bool rkci::BigSBinFloat::IsNegative() const
{
	return m_isNegative;
}

inline const rkci::BigUBinFloat_t &rkci::BigSBinFloat::GetUFloat() const
{
	return m_bigUFloat;
}

rkci::ResultRV<rkci::BigSBinFloat> rkci::BigSBinFloat::Clone() const
{
	RKC_CHECK_RV(rkci::BigUBinFloat_t, bigFloat, m_bigUFloat.Clone());

	return rkci::BigSBinFloat(rkci::Move(bigFloat), m_isNegative);
}
