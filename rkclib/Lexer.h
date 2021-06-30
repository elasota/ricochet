#pragma once

#include "CoreDefs.h"
#include "IStream.h"
#include "Result.h"
#include "StaticArray.h"
#include "Vector.h"
#include "Unicode.h"

namespace rkci
{
	struct IStream;
	struct IDestructible;

	enum class LexTokenType
	{
		kUnknown,

		kWhitespace,
		kName,
		kString,
		kNumber,
		kLineComment,
		kBlockComment,
		kEndOfLine,
		kEndOfFile,
		kPunctuation,
		kCharacterLiteral,
	};

	struct LexPosition
	{
		size_t m_line;
		size_t m_col;
		size_t m_filePos;

		LexPosition(size_t line, size_t col, size_t filePos);
	};

	struct LexToken
	{
		LexTokenType m_tokenType;
		const Vector<uint8_t, 4> &m_charBuffer;
		LexPosition m_startPos;
		LexPosition m_endPos;

		LexToken(LexTokenType tokenType, const Vector<uint8_t, 4> &charBuffer, const LexPosition &startPos, const LexPosition &endPos);
	};

	class Lexer
	{
	public:
		Lexer(IStream *stream, IAllocator *alloc);

		ResultRV<LexToken> GetNextToken();
		ResultRV<Optional<UnicodeChar_t>> PeekChar();
		void ConsumeChar();

	private:
		void SetNextCharacter(UnicodeChar_t nextChar, uint8_t numBytes);

		static const size_t kReadaheadCapacity = 4096;

		StaticArray<uint8_t, kReadaheadCapacity> m_byteBuffer;
		ArraySliceView<uint8_t> m_currentBytes;
		size_t m_leftoverBufferBytes;

		size_t m_bufReadOffset;
		size_t m_bufSize;

		bool m_isEOF;
		bool m_lastCharacterWasCR;
		size_t m_line;
		size_t m_col;
		size_t m_filePos;

		UnicodeChar_t m_nextChar;
		uint8_t m_nextCharSize;
		bool m_haveNextChar;

		IStream *m_stream;

		Vector<uint8_t, 4> m_charBuffer;
	};
}

inline rkci::LexToken::LexToken(LexTokenType tokenType, const Vector<uint8_t, 4> &charBuffer, const LexPosition &startPos, const LexPosition &endPos)
	: m_tokenType(tokenType)
	, m_charBuffer(charBuffer)
	, m_startPos(startPos)
	, m_endPos(endPos)
{
}

inline rkci::LexPosition::LexPosition(size_t line, size_t col, size_t filePos)
	: m_line(line)
	, m_col(col)
	, m_filePos(filePos)
{
}
