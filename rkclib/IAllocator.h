#pragma once

#include <stdint.h>
#include <stddef.h>

namespace rkci
{
	struct IAllocator
	{
		virtual void *Realloc(void *buf, size_t newSize) = 0;

		inline void *Alloc(size_t size) { return this->Realloc(nullptr, size); }
		inline void Release(void *ptr) { this->Realloc(ptr, 0); }
	};
}
