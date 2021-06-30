#pragma once
#ifndef __RICOCHET_COMPILER_LIB_H__
#define __RICOCHET_COMPILER_LIB_H__

#include <stdint.h>
#include <stddef.h>

#include "rkccore.h"
#include "ResultCode.h"

typedef struct RkcAllocatorSpec
{
	void *(*m_realloc)(void *userdata, void *buf, size_t newSize);
	void *m_userdata;
} RkcAllocatorSpec;

enum RkcStreamPermission
{
	RkcStreamPermission_Read = 1,
	RkcStreamPermission_Write = 2,
};

typedef struct RkcStreamFunctions
{
	// Reads up to a specified number of bytes into buf and returns the number of bytes read
	size_t (*m_read)(void *userdata, void *buf, size_t size);

	// Writes up to a specified number of bytes from buf and returns the number of bytes written
	size_t (*m_write)(void *userdata, const void *buf, size_t size);

	// Returns the current file position
	rkcUFilePos_t (*m_tell)(void *userdata);

	// Seeks to a position relative to the start of the file, returns non-zero if successful and 0 on failure
	int (*m_seekStart)(void *userdata, rkcUFilePos_t pos);

	// Seeks to a position relative to the end of the file, returns non-zero if successful and 0 on failure
	int (*m_seekEnd)(void *userdata, rkcFilePos_t pos);

	// Seeks to a position relative to the current location in the file, returns non-zero if successful and 0 on failure
	int (*m_seekCurrent)(void *userdata, rkcFilePos_t pos);

	// Closes the stream
	void (*m_close)(void *stream);
} RkcStreamFunctions;

typedef struct IRkcContext IRkcContext;

typedef struct RkcStreamSpec
{
	// Stream
	RkcStreamFunctions *m_functions;

	// File permissions
	int m_permissions;

	// Userdata pointer
	void *m_userdata;
} RkcStreamSpec;

extern "C" int RkcCreateContext(IRkcContext **outContext, const RkcAllocatorSpec *alloc);
extern "C" int RkcParseModule(const RkcStreamSpec *stream, const RkcAllocatorSpec *alloc);

#endif
