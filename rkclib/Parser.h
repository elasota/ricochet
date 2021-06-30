#pragma once

namespace rkci
{
	class Lexer;
	class ParseTree;
	struct IAllocator;
	template<class T> class ResultRV;

	namespace Ast
	{
		class ModuleFile;
	}

	namespace Parser
	{
		ResultRV<Ast::ModuleFile> Parse(rkci::Lexer &lexer, IAllocator &alloc);
	}
}
