#include "Result.h"

#include <assert.h>

#if RKC_IS_DEBUG

void rkci::Result::Unhandled()
{
	assert(!"Unhandled result");
}

void rkci::Result::AlreadyHandled()
{
	assert(!"Result already handled");
}

void rkci::Result::OnError()
{
	assert(!"Error result code");
}

#endif
