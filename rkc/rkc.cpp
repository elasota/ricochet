#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#include "../rkclib/rkclib.h"


RkcStreamSpec StreamFromCFile(FILE *f, bool isReadable, bool isWriteable);
void RkcTest(const RkcAllocatorSpec *allocSpec);

static void *ReallocThunk(void *userdata, void *buf, size_t newSize)
{
	return realloc(buf, newSize);
}

int main(int argc, const char **argv)
{
	FILE *f = fopen("D:\\experiments\\rkctests\\test.rk", "rb");
	RkcStreamSpec streamSpec = StreamFromCFile(f, true, false);

	RkcAllocatorSpec allocSpec;
	allocSpec.m_realloc = ReallocThunk;
	allocSpec.m_userdata = nullptr;

	RkcTest(&allocSpec);

	//RkcTestParseStream(&streamSpec, &allocSpec);
	return 0;
}
