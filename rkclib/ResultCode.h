#pragma once

namespace rkc
{
	namespace ResultCodes
	{
		enum ResultCode
		{
			kOK = 0,

			kOutOfMemory,
			kIntegerOverflow,
			kMalformedNumber,

			kLexMalformedNumber,
			kLexGarbageCharacter,
			kLexUnexpectedEndOfFile,
			kLexUnexpectedEndOfLine,
			kLexMalformedString,
			kLexMalformedCharacterLiteral,
			kLexInvalidUnicode,
			kLexInvalidEscape,

			kInternalError,
			kNotYetImplemented,
		};
	}

	typedef ResultCodes::ResultCode ResultCode_t;
}
