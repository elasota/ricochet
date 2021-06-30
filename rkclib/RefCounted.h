#pragma once

namespace rkci
{
	struct IRefCounted
	{
		virtual void AddRef() = 0;
		virtual void DecRef() = 0;
	};
}
