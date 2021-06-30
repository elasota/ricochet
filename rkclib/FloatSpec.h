#pragma once

#include "CoreDefs.h"

namespace rkci
{
	class FloatSpec
	{
	public:
		FloatSpec(uint16_t numExponentBits, uint16_t numMantissaBits, int16_t exponentOfOne, bool supportDenormals, bool supportNans);

		uint16_t GetExponentBits() const;
		uint16_t GetMantissaBits() const;
		int16_t GetExponentOfOne() const;
		bool SupportsDenormals() const;
		bool SupportsNans() const;

	private:
		uint16_t m_numExponentBits;
		uint16_t m_numMantissaBits;
		int16_t m_exponentOfOne;
		bool m_supportDenormals;
		bool m_supportNans;
	};
}

inline rkci::FloatSpec::FloatSpec(uint16_t numExponentBits, uint16_t numMantissaBits, int16_t exponentOfOne, bool supportDenormals, bool supportsNans)
	: m_numExponentBits(numExponentBits)
	, m_numMantissaBits(numMantissaBits)
	, m_exponentOfOne(exponentOfOne)
	, m_supportDenormals(supportDenormals)
	, m_supportNans(supportsNans)
{
}

inline uint16_t rkci::FloatSpec::GetExponentBits() const
{
	return m_numExponentBits;
}

inline uint16_t rkci::FloatSpec::GetMantissaBits() const
{
	return m_numMantissaBits;
}

inline int16_t rkci::FloatSpec::GetExponentOfOne() const
{
	return m_exponentOfOne;
}

inline bool rkci::FloatSpec::SupportsDenormals() const
{
	return m_supportDenormals;
}


inline bool rkci::FloatSpec::SupportsNans() const
{
	return m_supportDenormals;
}
