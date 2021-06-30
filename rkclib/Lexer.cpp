#include "Lexer.h"
#include "CharCodes.h"
#include "Unicode.h"

#include "ArrayTools.h"
#include "Optional.h"

namespace rkci
{
	namespace LexerLocal
	{
		enum class TokenParseState
		{
			kStart,
			kWhitespace,
			kEndOfLine,
			kPunctuation,
			kText,
			kNumber,
			kLineComment,
			kBlockComment,
			kQuotedString,
		};

		enum class CharacterCategory
		{
			kGarbage,
			kWhitespace,
			kEndOfLine,
			kPunctuation,
			kText,
			kDigit,
		};

		CharacterCategory CategorizeCharacter(UnicodeChar_t uchar);

		Result ConsumeChar(Vector<uint8_t, 4> &chars, UnicodeChar_t uchar, Lexer &lexer)
		{
#if RKC_ASSERTS_ENABLED
			RKC_CHECK_RV(Optional<UnicodeChar_t>, token, lexer.PeekChar());
			RKC_ASSERT(token.IsSet() && token.Get() == uchar);
#endif

			StaticArray<uint8_t, rkci::Unicode::Utf8::kMaxEncodedBytes> encoded;
			size_t charsEmitted = rkci::Unicode::Utf8::Encode(encoded.GetSlice(), uchar);

			for (size_t i = 0; i < charsEmitted; i++)
			{
				RKC_CHECK(chars.Append(encoded[i]));
			}

			lexer.ConsumeChar();
			return Result::Ok();
		}

		Result ParseWhitespace(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kWhitespace;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());

				if (!ucharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t uchar = ucharOpt.Get();

				const CharacterCategory category = CategorizeCharacter(uchar);
				if (category != CharacterCategory::kWhitespace)
					return Result::Ok();
				else
				{
					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
				}
			}
		}

		Result ParseEndOfLine(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType, UnicodeChar_t firstChar)
		{
			outTokenType = LexTokenType::kEndOfLine;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());
				if (!ucharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t uchar = ucharOpt.Get();

				if (CategorizeCharacter(uchar) == CharacterCategory::kEndOfLine)
				{
					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
				}
				else
					return Result::Ok();
			}
		}

		Result ParseLineComment(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kLineComment;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, nextCharOpt, lexer.PeekChar());
				if (!nextCharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t nextChar = nextCharOpt.Get();

				if (CategorizeCharacter(nextChar) == CharacterCategory::kEndOfLine)
					return Result::Ok();
				else
				{
					RKC_CHECK(ConsumeChar(chars, nextChar, lexer));
				}
			}
		}

		Result ParseBlockComment(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			bool lastWasAsterisk = false;
			outTokenType = LexTokenType::kLineComment;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, nextCharOpt, lexer.PeekChar());

				if (!nextCharOpt.IsSet())
					return rkc::ResultCodes::kLexUnexpectedEndOfFile;

				const UnicodeChar_t nextChar = nextCharOpt.Get();

				RKC_CHECK(ConsumeChar(chars, nextChar, lexer));

				if (nextChar == CharCodes::kAsterisk)
					lastWasAsterisk = true;
				else if (nextChar == CharCodes::kSlash)
					return Result::Ok();
			}
		}

		Result ParseCharacterEscape(Lexer &lexer, Vector<uint8_t, 4> &chars, rkc::ResultCode_t failureCode)
		{
			RKC_CHECK_RV(Optional<UnicodeChar_t>, escapeControlOpt, lexer.PeekChar());

			if (!escapeControlOpt.IsSet())
				return rkc::ResultCodes::kLexUnexpectedEndOfFile;

			const UnicodeChar_t escapeControl = escapeControlOpt.Get();

			RKC_CHECK(ConsumeChar(chars, escapeControl, lexer));

			size_t numHexDigits = 0;
			switch (escapeControl)
			{
			case CharCodes::kLowercaseU:
				numHexDigits = 4;
				break;
			case CharCodes::kUppercaseU:
				numHexDigits = 8;
				break;
			case CharCodes::kUppercaseX:
				numHexDigits = 2;
				break;
			case CharCodes::kSingleQuote:
			case CharCodes::kDoubleQuote:
			case CharCodes::kBackslash:
			case CharCodes::kDigit0:
			case CharCodes::kLowercaseA:
			case CharCodes::kLowercaseB:
			case CharCodes::kLowercaseF:
			case CharCodes::kLowercaseN:
			case CharCodes::kLowercaseR:
			case CharCodes::kLowercaseT:
			case CharCodes::kLowercaseV:
				return Result::Ok();
			default:
				return rkc::ResultCodes::kLexInvalidEscape;
			}

			for (size_t i = 0; i < numHexDigits; i++)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, hexDigitOpt, lexer.PeekChar());

				if (!hexDigitOpt.IsSet())
					return rkc::ResultCodes::kLexUnexpectedEndOfFile;

				const UnicodeChar_t hexDigit = hexDigitOpt.Get();

				const bool isHexDigit = (hexDigit >= CharCodes::kLowercaseA && hexDigit <= CharCodes::kLowercaseF)
					|| (hexDigit >= CharCodes::kUppercaseA && hexDigit <= CharCodes::kUppercaseF)
					|| (hexDigit >= CharCodes::kDigit0 && hexDigit <= CharCodes::kDigit9);

				if (!isHexDigit)
					return rkc::ResultCodes::kLexInvalidEscape;

				RKC_CHECK(ConsumeChar(chars, hexDigit, lexer));
			}

			return Result::Ok();
		}

		Result ParseCharacterLiteral(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kCharacterLiteral;

			RKC_CHECK_RV(Optional<UnicodeChar_t>, charStartOpt, lexer.PeekChar());
			if (!charStartOpt.IsSet())
				return rkc::ResultCodes::kLexUnexpectedEndOfFile;

			const UnicodeChar_t charStart = charStartOpt.Get();

			if (charStart == CharCodes::kSingleQuote || charStart == CharCodes::kDoubleQuote)
				return rkc::ResultCodes::kLexMalformedCharacterLiteral;

			RKC_CHECK(ConsumeChar(chars, charStart, lexer));

			if (charStart == CharCodes::kBackslash)
			{
				RKC_CHECK(ParseCharacterEscape(lexer, chars, rkc::ResultCodes::kLexMalformedCharacterLiteral));
			}
			else
			{
				switch (CategorizeCharacter(charStart))
				{
				case CharacterCategory::kGarbage:
					return rkc::ResultCodes::kLexGarbageCharacter;
				case CharacterCategory::kEndOfLine:
					return rkc::ResultCodes::kLexUnexpectedEndOfLine;
				case CharacterCategory::kWhitespace:
				case CharacterCategory::kPunctuation:
				case CharacterCategory::kText:
				case CharacterCategory::kDigit:
					break;
				default:
					RKC_ASSERT(false);
					return rkc::ResultCodes::kInternalError;
				}
			}

			RKC_CHECK_RV(Optional<UnicodeChar_t>, endQuoteOpt, lexer.PeekChar());

			if (!endQuoteOpt.IsSet())
				return rkc::ResultCodes::kLexUnexpectedEndOfFile;

			if (endQuoteOpt.Get() != CharCodes::kSingleQuote)
				return rkc::ResultCodes::kLexMalformedCharacterLiteral;

			RKC_CHECK(ConsumeChar(chars, charStart, lexer));

			return Result::Ok();
		}

		Result ParseQuotedString(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kString;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, charNextOpt, lexer.PeekChar());

				if (!charNextOpt.IsSet())
					return rkc::ResultCodes::kLexUnexpectedEndOfFile;

				const UnicodeChar_t charNext = charNextOpt.Get();

				RKC_CHECK(ConsumeChar(chars, charNext, lexer));

				if (charNext == CharCodes::kDoubleQuote)
					return Result::Ok();

				if (charNext == CharCodes::kBackslash)
				{
					RKC_CHECK(ParseCharacterEscape(lexer, chars, rkc::ResultCodes::kLexMalformedString));
				}
				else
				{
					switch (CategorizeCharacter(charNext))
					{
					case CharacterCategory::kGarbage:
						return rkc::ResultCodes::kLexGarbageCharacter;
					case CharacterCategory::kEndOfLine:
						return rkc::ResultCodes::kLexUnexpectedEndOfLine;
					case CharacterCategory::kWhitespace:
					case CharacterCategory::kPunctuation:
					case CharacterCategory::kText:
					case CharacterCategory::kDigit:
						break;
					default:
						RKC_ASSERT(false);
						return rkc::ResultCodes::kInternalError;
					}
				}
			}
		}

		Result ParsePunctuation(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType, UnicodeChar_t firstChar)
		{
			outTokenType = LexTokenType::kPunctuation;

			if (firstChar == CharCodes::kSingleQuote)
				return ParseCharacterLiteral(lexer, chars, outTokenType);

			if (firstChar == CharCodes::kDoubleQuote)
				return ParseQuotedString(lexer, chars, outTokenType);

			uint8_t permittedNextChars[3];
			size_t numPermittedNextChars = 0;

			switch (firstChar)
			{
			case CharCodes::kAmpersand:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kAmpersand;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kVerticalBar:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kVerticalBar;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kPlus:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kPlus;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kMinus:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kMinus;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kEqual:
			case CharCodes::kExclamation:
			case CharCodes::kPercent:
			case CharCodes::kCaret:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kLess:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kLess;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kAsterisk:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				break;
			case CharCodes::kSlash:
				permittedNextChars[numPermittedNextChars++] = CharCodes::kEqual;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kSlash;
				permittedNextChars[numPermittedNextChars++] = CharCodes::kAsterisk;
				break;
			default:
				return Result::Ok();
			}

			RKC_ASSERT(numPermittedNextChars >= 1);

			RKC_CHECK_RV(Optional<UnicodeChar_t>, secondCharOpt, lexer.PeekChar());
			if (!secondCharOpt.IsSet())
				return Result::Ok();

			const UnicodeChar_t secondChar = secondCharOpt.Get();

			bool isMatch = false;
			for (size_t i = 0; i < numPermittedNextChars; i++)
			{
				if (permittedNextChars[i] == secondChar)
				{
					isMatch = true;
					break;
				}
			}

			if (isMatch)
			{
				RKC_CHECK(ConsumeChar(chars, secondChar, lexer));
			}
			else
				return Result::Ok();

			if (firstChar == CharCodes::kSlash)
			{
				if (secondChar == CharCodes::kSlash)
					return ParseLineComment(lexer, chars, outTokenType);
				if (secondChar == CharCodes::kAsterisk)
					return ParseBlockComment(lexer, chars, outTokenType);
			}

			if (firstChar == CharCodes::kLess && secondChar == CharCodes::kLess)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, thirdCharOpt, lexer.PeekChar());
				if (!thirdCharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t thirdChar = thirdCharOpt.Get();

				if (thirdChar == CharCodes::kEqual)
				{
					RKC_CHECK(ConsumeChar(chars, thirdChar, lexer));
				}
				else
					return Result::Ok();
			}

			return Result::Ok();
		}

		Result ParseIdentifier(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kName;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());
				if (!ucharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t uchar = ucharOpt.Get();
				const CharacterCategory category = CategorizeCharacter(uchar);

				switch (category)
				{
				case CharacterCategory::kGarbage:
					return rkc::ResultCodes::kLexGarbageCharacter;
				case CharacterCategory::kWhitespace:
				case CharacterCategory::kEndOfLine:
				case CharacterCategory::kPunctuation:
					return Result::Ok();
				case CharacterCategory::kText:
				case CharacterCategory::kDigit:
					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
					break;
				default:
					return rkc::ResultCodes::kInternalError;
				}
			}
		}

		Result ParseHexNumber(Lexer &lexer, Vector<uint8_t, 4> &chars)
		{
			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());
				if (!ucharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t uchar = ucharOpt.Get();
				const CharacterCategory category = CategorizeCharacter(uchar);

				switch (category)
				{
				case CharacterCategory::kDigit:
					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
					break;
				case CharacterCategory::kGarbage:
					return rkc::ResultCodes::kLexGarbageCharacter;
				case CharacterCategory::kWhitespace:
				case CharacterCategory::kEndOfLine:
				case CharacterCategory::kPunctuation:
					return Result::Ok();
				case CharacterCategory::kText:
					if ((uchar < CharCodes::kLowercaseA || uchar > CharCodes::kLowercaseF) && (uchar < CharCodes::kUppercaseA || uchar > CharCodes::kUppercaseF))
						return rkc::ResultCodes::kLexMalformedNumber;

					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
					break;
				default:
					return rkc::ResultCodes::kInternalError;
				}

				return ParseHexNumber(lexer, chars);
			}
		}

		Result ParseNumber(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			outTokenType = LexTokenType::kNumber;

			bool mayHaveDigits = true;
			bool mayHaveHexDigits = false;
			bool mayHaveDecimal = true;
			bool mayHaveExponent = true;
			bool mayHaveSuffix = true;
			bool mayHaveHex = true;

			for (;;)
			{
				RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());
				if (!ucharOpt.IsSet())
					return Result::Ok();

				const UnicodeChar_t uchar = ucharOpt.Get();

				if (mayHaveHex)
				{
					if (uchar == CharCodes::kLowercaseX)
					{
						RKC_CHECK(ConsumeChar(chars, uchar, lexer));

						RKC_CHECK_RV(Optional<UnicodeChar_t>, firstDigitOpt, lexer.PeekChar());
						if (!firstDigitOpt.IsSet() || CategorizeCharacter(firstDigitOpt.Get()) != CharacterCategory::kDigit)
							return rkc::ResultCodes::kLexMalformedNumber;

						RKC_CHECK(ConsumeChar(chars, firstDigitOpt.Get(), lexer));
						return ParseHexNumber(lexer, chars);
					}
					else
						mayHaveHex = false;
				}

				const CharacterCategory category = CategorizeCharacter(uchar);

				switch (category)
				{
				case CharacterCategory::kGarbage:
					return rkc::ResultCodes::kLexGarbageCharacter;
				case CharacterCategory::kWhitespace:
				case CharacterCategory::kEndOfLine:
				case CharacterCategory::kPunctuation:
					if (uchar == CharCodes::kPeriod && mayHaveDecimal)
					{
						RKC_CHECK(ConsumeChar(chars, uchar, lexer));
						mayHaveDecimal = false;

						RKC_CHECK_RV(Optional<UnicodeChar_t>, firstDecimalDigitOpt, lexer.PeekChar());
						if (!firstDecimalDigitOpt.IsSet())
							return rkc::ResultCodes::kLexUnexpectedEndOfFile;

						const UnicodeChar_t firstDecimalDigit = firstDecimalDigitOpt.Get();
						if (CategorizeCharacter(firstDecimalDigit) == CharacterCategory::kDigit)
						{
							RKC_CHECK(ConsumeChar(chars, firstDecimalDigit, lexer));
						}
						else
							return rkc::ResultCodes::kLexMalformedNumber;
					}
					else
						return Result::Ok();
					break;
				case CharacterCategory::kDigit:
					if (!mayHaveDigits)
						return rkc::ResultCodes::kLexMalformedNumber;
					RKC_CHECK(ConsumeChar(chars, uchar, lexer));
					break;
				case CharacterCategory::kText:
					if (uchar == CharCodes::kLowercaseF || uchar == CharCodes::kUppercaseF || uchar == CharCodes::kLowercaseD || uchar == CharCodes::kUppercaseD)
					{
						if (!mayHaveSuffix)
							return rkc::ResultCodes::kLexMalformedNumber;

						RKC_CHECK(ConsumeChar(chars, uchar, lexer));

						mayHaveDigits = false;
						mayHaveDecimal = false;
						mayHaveExponent = false;
						mayHaveSuffix = false;
					}
					else if (uchar == CharCodes::kLowercaseE || uchar == CharCodes::kUppercaseE)
					{
						if (!mayHaveExponent)
							return rkc::ResultCodes::kLexMalformedNumber;

						RKC_CHECK(ConsumeChar(chars, uchar, lexer));

						RKC_CHECK_RV(Optional<UnicodeChar_t>, firstExponentCharOpt, lexer.PeekChar());
						if (!firstExponentCharOpt.IsSet())
							return rkc::ResultCodes::kLexUnexpectedEndOfFile;

						const UnicodeChar_t firstExponentChar = firstExponentCharOpt.Get();
						if (firstExponentChar == CharCodes::kMinus)
						{
							RKC_CHECK(ConsumeChar(chars, firstExponentChar, lexer));

							RKC_CHECK_RV(Optional<UnicodeChar_t>, firstExponentDigitOpt, lexer.PeekChar());
							if (!firstExponentDigitOpt.IsSet())
								return rkc::ResultCodes::kLexUnexpectedEndOfFile;

							const UnicodeChar_t firstExponentDigit = firstExponentDigitOpt.Get();
							if (CategorizeCharacter(firstExponentDigit) == CharacterCategory::kDigit)
							{
								RKC_CHECK(ConsumeChar(chars, firstExponentDigit, lexer));
							}
							else
								return rkc::ResultCodes::kLexMalformedNumber;
						}
						else if (CategorizeCharacter(firstExponentChar) == CharacterCategory::kDigit)
						{
							RKC_CHECK(ConsumeChar(chars, firstExponentChar, lexer));
						}
						else
							return rkc::ResultCodes::kLexMalformedNumber;

						mayHaveDecimal = false;
						mayHaveExponent = false;
					}
					else
					{
						return rkc::ResultCodes::kLexMalformedNumber;
					}
					break;
				default:
					return rkc::ResultCodes::kInternalError;
				}
			}
		}

		Result ParseToken(Lexer &lexer, Vector<uint8_t, 4> &chars, LexTokenType &outTokenType)
		{
			RKC_CHECK_RV(Optional<UnicodeChar_t>, ucharOpt, lexer.PeekChar());
			if (!ucharOpt.IsSet())
			{
				outTokenType = LexTokenType::kEndOfFile;
				return Result::Ok();
			}

			const UnicodeChar_t uchar = ucharOpt.Get();
			const CharacterCategory startCategory = CategorizeCharacter(uchar);
			RKC_CHECK(ConsumeChar(chars, uchar, lexer));

			switch (startCategory)
			{
			case CharacterCategory::kGarbage:
				return Result(rkc::ResultCodes::kLexGarbageCharacter);
			case CharacterCategory::kWhitespace:
				return ParseWhitespace(lexer, chars, outTokenType);
			case CharacterCategory::kEndOfLine:
				return ParseEndOfLine(lexer, chars, outTokenType, uchar);
			case CharacterCategory::kPunctuation:
				return ParsePunctuation(lexer, chars, outTokenType, uchar);
			case CharacterCategory::kText:
				return ParseIdentifier(lexer, chars, outTokenType);
			case CharacterCategory::kDigit:
				return ParseNumber(lexer, chars, outTokenType);
			default:
				RKC_ASSERT(false);
				return rkc::ResultCodes::kInternalError;
			};
		}
	}
}


rkci::LexerLocal::CharacterCategory rkci::LexerLocal::CategorizeCharacter(UnicodeChar_t uchar)
{
	switch (uchar)
	{
	case CharCodes::kSpace:
	case CharCodes::kNBSP:
	case CharCodes::kOghamSpaceMark:
	case CharCodes::kEnQuad:
	case CharCodes::kEmQuad:
	case CharCodes::kEnSpace:
	case CharCodes::kEmSpace:
	case CharCodes::kThreePerEmSpace:
	case CharCodes::kFourPerEmSpace:
	case CharCodes::kSixPerEmSpace:
	case CharCodes::kFigureSpace:
	case CharCodes::kPunctuationSpace:
	case CharCodes::kThinSpace:
	case CharCodes::kHairSpace:
	case CharCodes::kNNBSP:
	case CharCodes::kMMSP:
	case CharCodes::kIdeographicSpace:
	case CharCodes::kTab:
	case CharCodes::kVTab:
	case CharCodes::kFormFeed:
		return CharacterCategory::kWhitespace;
	case CharCodes::kCarriageReturn:
	case CharCodes::kLineFeed:
	case CharCodes::kNextLine:
	case CharCodes::kLineSeparator:
	case CharCodes::kParagraphSeparator:
		return CharacterCategory::kEndOfLine;
	default:
		break;
	}

	if (uchar <= 32)
		return CharacterCategory::kGarbage;

	if (uchar >= 33 && uchar <= 47)
		return CharacterCategory::kPunctuation;

	if (uchar >= 48 && uchar <= 57)
		return CharacterCategory::kDigit;

	if (uchar >= 58 && uchar <= 64)
		return CharacterCategory::kPunctuation;

	if (uchar >= 65 && uchar <= 90)
		return CharacterCategory::kText;

	if (uchar == 95)
		return CharacterCategory::kText;

	if (uchar >= 91 && uchar <= 96)
		return CharacterCategory::kPunctuation;

	if (uchar >= 97 && uchar <= 123)
		return CharacterCategory::kText;

	if (uchar >= 123 && uchar <= 126)
		return CharacterCategory::kPunctuation;

	if (uchar == 127)
		return CharacterCategory::kGarbage;

	return CharacterCategory::kText;
}

rkci::Lexer::Lexer(IStream *stream, IAllocator *alloc)
	: m_stream(stream)
	, m_bufReadOffset(0)
	, m_bufSize(0)
	, m_charBuffer(alloc)
	, m_leftoverBufferBytes(0)
	, m_isEOF(false)
	, m_lastCharacterWasCR(false)
	, m_line(0)
	, m_col(0)
	, m_filePos(0)
	, m_nextChar(0)
	, m_nextCharSize(0)
	, m_haveNextChar(false)
{
}

rkci::ResultRV<rkci::LexToken> rkci::Lexer::GetNextToken()
{
	RKC_CHECK(m_charBuffer.Resize(0));

	const LexPosition startPos(m_line, m_col, m_filePos);

	LexTokenType tokenType = LexTokenType::kUnknown;
	RKC_CHECK(LexerLocal::ParseToken(*this, m_charBuffer, tokenType));

	const LexPosition endPos(m_line, m_col, m_filePos);

	return LexToken(tokenType, m_charBuffer, startPos, endPos);
}

rkci::ResultRV<rkci::Optional<rkci::UnicodeChar_t>> rkci::Lexer::PeekChar()
{
	if (m_haveNextChar)
		return rkci::Optional<rkci::UnicodeChar_t>(m_nextChar);

	if (m_isEOF)
		return rkci::Optional<rkci::UnicodeChar_t>();

	if (m_currentBytes.Count() > 0)
	{
		const rkci::Unicode::UnicodeDecodeResult decodeResult = rkci::Unicode::Utf8::Decode(m_currentBytes);
		if (decodeResult.m_decodeResultType == rkci::Unicode::DecodeResultType::kMalformed)
			return rkc::ResultCodes::kLexInvalidUnicode;
		if (decodeResult.m_decodeResultType == rkci::Unicode::DecodeResultType::kOK)
		{
			SetNextCharacter(decodeResult.m_char, decodeResult.m_countDigested);
			return rkci::Optional<rkci::UnicodeChar_t>(m_nextChar);
		}
	}

	// Need more bytes
	rkci::ArrayTools::Move(m_byteBuffer.GetSlice(), m_currentBytes);

	const size_t startOffset = m_currentBytes.Count();
	const size_t readAdditional = m_stream->Read(&m_byteBuffer[startOffset], m_byteBuffer.Count() - startOffset);

	m_currentBytes = m_byteBuffer.GetSlice().Subrange(0, startOffset + readAdditional);

	if (readAdditional == 0)
	{
		// Partial Unicode character didn't get any extra bytes
		if (startOffset > 0)
			return rkc::ResultCodes::kLexInvalidUnicode;

		m_isEOF = true;
		return rkci::Optional<rkci::UnicodeChar_t>();
	}

	const rkci::Unicode::UnicodeDecodeResult decodeResult = rkci::Unicode::Utf8::Decode(m_currentBytes);
	if (decodeResult.m_decodeResultType == rkci::Unicode::DecodeResultType::kOK)
	{
		SetNextCharacter(decodeResult.m_char, decodeResult.m_countDigested);
		return rkci::Optional<rkci::UnicodeChar_t>(m_nextChar);
	}

	// Partial result that couldn't get any more data
	m_isEOF = true;
	return rkc::ResultCodes::kLexInvalidUnicode;
}

void rkci::Lexer::ConsumeChar()
{
	RKC_ASSERT(m_haveNextChar);
	m_haveNextChar = false;

	if (LexerLocal::CategorizeCharacter(m_nextChar) == LexerLocal::CharacterCategory::kEndOfLine)
	{
		if (m_nextChar != CharCodes::kLineFeed || !m_lastCharacterWasCR)
		{
			m_line++;
			m_col = 0;
		}

		m_lastCharacterWasCR = (m_nextChar == CharCodes::kCarriageReturn);
	}
	else
		m_col++;

	m_filePos += m_nextCharSize;
	m_currentBytes = m_currentBytes.Subrange(m_nextCharSize, m_currentBytes.Count() - m_nextCharSize);

	m_haveNextChar = false;
}

void rkci::Lexer::SetNextCharacter(UnicodeChar_t nextChar, uint8_t numBytes)
{
	RKC_ASSERT(!m_haveNextChar);

	m_haveNextChar = true;
	m_nextChar = nextChar;
	m_nextCharSize = numBytes;
}
