#pragma once

#include <stdint.h>
#include <stddef.h>

#include "Optional.h"

namespace rkci
{
	template<class... T> class Tuple;
	template<class T> class ArraySliceView;

	typedef uint32_t UnicodeChar_t;

	namespace Unicode
	{
		enum class DecodeResultType
		{
			kOK,
			kIncomplete,
			kMalformed,
		};

		struct UnicodeDecodeResult
		{
			DecodeResultType m_decodeResultType;
			UnicodeChar_t m_char;
			uint8_t m_countDigested;
		};

		namespace Utf8
		{
			static const unsigned int kMaxEncodedBytes = 4;

			UnicodeDecodeResult Decode(ArraySliceView<const uint8_t> bytes);
			size_t Encode(ArraySliceView<uint8_t> bytes, UnicodeChar_t codePoint);
		}

		namespace Utf16
		{
			UnicodeDecodeResult Decode(ArraySliceView<const uint16_t> words);
			size_t Encode(ArraySliceView<uint16_t> words, UnicodeChar_t codePoint);
		}
	}
}
