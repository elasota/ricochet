#pragma once

#include "rkclib.h"
#include "rkccore.h"

namespace rkci
{
	struct IStream
	{
		virtual size_t Read(void *buf, size_t size) = 0;
		virtual size_t Write(void *buf, size_t size) = 0;
		virtual rkcUFilePos_t Tell() const = 0;
		virtual bool SeekStart(rkcUFilePos_t pos) = 0;
		virtual bool SeekEnd(rkcFilePos_t pos) = 0;
		virtual bool SeekCurrent(rkcFilePos_t pos) = 0;
		virtual bool IsReadable() const = 0;
		virtual bool IsWritable() const = 0;
		virtual void Close() = 0;
	};
}
