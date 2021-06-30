#include "Result.h"

namespace rkci
{
	struct IAllocator;

	namespace Tests
	{
		Result BigAtof(IAllocator &alloc);
	}
}

static rkci::Result RkcTestInternal(rkci::IAllocator &alloc)
{
	RKC_CHECK(rkci::Tests::BigAtof(alloc));

	return rkci::Result::Ok();
}

int RkcRunTests(rkci::IAllocator &alloc)
{
	rkci::Result result((RkcTestInternal(alloc)));
	result.Handle();

	return static_cast<int>(result.GetCode());
}

