#include "Unicode.h"
#include "ArraySliceView.h"

rkci::Unicode::UnicodeDecodeResult rkci::Unicode::Utf8::Decode(ArraySliceView<const uint8_t> bytes)
{
	rkci::Unicode::UnicodeDecodeResult decodeResult;
	decodeResult.m_char = UnicodeChar_t(0);
	decodeResult.m_decodeResultType = DecodeResultType::kMalformed;
	decodeResult.m_countDigested = 0;

	const size_t availableBytes = bytes.Count();

	if (availableBytes <= 0)
	{
		decodeResult.m_decodeResultType = DecodeResultType::kIncomplete;
		return decodeResult;
	}

	const uint8_t firstChar = bytes[0];
	if ((firstChar & 0x80) == 0x00)
	{
		decodeResult.m_countDigested = 1;
		decodeResult.m_char = firstChar;
		decodeResult.m_decodeResultType = DecodeResultType::kOK;
		return decodeResult;
	}

	uint8_t sz = 0;
	UnicodeChar_t codePoint = 0;
	UnicodeChar_t minCodePoint = 0;
	if ((firstChar & 0xe0) == 0xc0)
	{
		sz = 2;
		minCodePoint = 0x80;
		codePoint = (firstChar & 0x1f);
	}
	else if ((firstChar & 0xf0) == 0xe0)
	{
		sz = 3;
		minCodePoint = 0x800;
		codePoint = (firstChar & 0x0f);
	}
	else if ((firstChar & 0xf8) == 0xf0)
	{
		sz = 4;
		minCodePoint = 0x10000;
		codePoint = (firstChar & 0x07);
	}
	else
		return decodeResult;

	if (availableBytes < sz)
	{
		decodeResult.m_decodeResultType = DecodeResultType::kIncomplete;
		return decodeResult;
	}

	for (size_t auxByteIndex = 1; auxByteIndex < sz; auxByteIndex++)
	{
		const uint8_t auxByte = bytes[auxByteIndex];
		if ((auxByte & 0xc0) != 0x80)
			return decodeResult;

		codePoint = (codePoint << 6) | (auxByte & 0x3f);
	}

	if (codePoint < minCodePoint || codePoint > 0x10ffff)
		return decodeResult;

	if (codePoint >= 0xd800 && codePoint <= 0xdfff)
		return decodeResult;

	decodeResult.m_countDigested = sz;
	decodeResult.m_char = codePoint;
	decodeResult.m_decodeResultType = DecodeResultType::kOK;

	return decodeResult;
}

rkci::Unicode::UnicodeDecodeResult rkci::Unicode::Utf16::Decode(ArraySliceView<const uint16_t> words)
{
	rkci::Unicode::UnicodeDecodeResult decodeResult;
	decodeResult.m_char = UnicodeChar_t(0);
	decodeResult.m_decodeResultType = DecodeResultType::kMalformed;
	decodeResult.m_countDigested = 0;

	const size_t availableWords = words.Count();

	if (availableWords <= 0)
	{
		decodeResult.m_decodeResultType = DecodeResultType::kIncomplete;
		return decodeResult;
	}

	const uint16_t firstWord = words[0];

	if (firstWord <= 0xd7ff || firstWord >= 0xe000)
	{
		decodeResult.m_countDigested = 1;
		decodeResult.m_char = firstWord;
		decodeResult.m_decodeResultType = DecodeResultType::kOK;
		return decodeResult;
	}

	// Unpaired low surrogate
	if (firstWord >= 0xdc00)
		return decodeResult;

	if (availableWords < 2)
	{
		decodeResult.m_decodeResultType = DecodeResultType::kIncomplete;
		return decodeResult;
	}

	const uint16_t secondWord = words[1];

	if (secondWord < 0xdc00 || secondWord >= 0xe000)
		return decodeResult;

	const uint16_t highBits = (firstWord & 0x3ff);
	const uint16_t lowBits = (secondWord & 0x3ff);

	decodeResult.m_countDigested = 2;
	decodeResult.m_char = (highBits << 10) + lowBits + 0x10000;
	decodeResult.m_decodeResultType = DecodeResultType::kOK;

	return decodeResult;
}

size_t rkci::Unicode::Utf8::Encode(ArraySliceView<uint8_t> bytes, UnicodeChar_t codePoint)
{
	if ((codePoint & 0x1fffff) != codePoint)
		return 0;

	if (codePoint >= 0xd800 && codePoint < 0xdfff)
		return 0;

	uint8_t signalBits = 0;
	size_t numBytes = 0;
	if (codePoint < 0x0080)
	{
		numBytes = 1;
		signalBits = 0;
	}
	else if (codePoint < 0x0800)
	{
		numBytes = 2;
		signalBits = 0xc0;
	}
	else if (codePoint < 0x10000)
	{
		numBytes = 3;
		signalBits = 0xe0;
	}
	else
	{
		numBytes = 4;
		signalBits = 0xf0;
	}

	bytes[0] = static_cast<uint8_t>((codePoint >> (6 * (numBytes - 1))) | signalBits);

	for (size_t i = 1; i < numBytes; i++)
	{
		const UnicodeChar_t isolate = ((codePoint >> (6 * (numBytes - 1 - i))) & 0x3f) | 0x80;
		bytes[i] = static_cast<uint8_t>(isolate);
	}

	return numBytes;
}

size_t rkci::Unicode::Utf16::Encode(ArraySliceView<uint16_t> words, UnicodeChar_t codePoint)
{
	if ((codePoint & 0x1fffff) != codePoint)
		return 0;

	if (codePoint >= 0xd800 && codePoint < 0xdfff)
		return 0;

	if (codePoint <= 0xd7ff || codePoint >= 0xe000)
	{
		words[0] = static_cast<uint16_t>(codePoint);
		return 1;
	}

	const UnicodeChar_t codePointBits = (codePoint - 0x10000) & 0xfffff;
	const uint16_t lowBits = (codePointBits & 0x3ff);
	const uint16_t highBits = ((codePointBits >> 10) & 0x3ff);

	words[0] = (0xd800 + highBits);
	words[1] = (0xdc00 + lowBits);

	return 2;
}
