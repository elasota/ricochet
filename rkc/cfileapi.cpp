#include <stdio.h>

#include "../rkclib/rkclib.h"

static size_t CFileRead(void *f, void *buf, size_t size)
{
	const size_t numRead = fread(buf, 1, size, static_cast<FILE*>(f));
	return numRead;
}

static size_t CFileWrite(void *f, const void *buf, size_t size)
{
	const size_t numWritten = fwrite(buf, 1, size, static_cast<FILE*>(f));
	return numWritten;
}

static rkcUFilePos_t CFileTell(void *f)
{
	return static_cast<rkcUFilePos_t>(ftell(static_cast<FILE*>(f)));
}

static int CFileSeekStart(void *f, rkcUFilePos_t pos)
{
	return fseek(static_cast<FILE*>(f), static_cast<rkcUFilePos_t>(pos), SEEK_SET) != 0;
}

static int CFileSeekEnd(void *f, rkcFilePos_t pos)
{
	return fseek(static_cast<FILE*>(f), static_cast<rkcUFilePos_t>(pos), SEEK_END) != 0;
}

static int CFileSeekCurrent(void *f, rkcFilePos_t pos)
{
	return fseek(static_cast<FILE*>(f), static_cast<rkcUFilePos_t>(pos), SEEK_CUR) != 0;
}

static void CFileClose(void *f)
{
	fclose(static_cast<FILE*>(f));
}


static RkcStreamFunctions gs_streamFuncs =
{
	CFileRead,
	CFileWrite,
	CFileTell,
	CFileSeekStart,
	CFileSeekEnd,
	CFileSeekCurrent,
	CFileClose,
};

RkcStreamSpec StreamFromCFile(FILE *f, bool isReadable, bool isWriteable)
{
	RkcStreamSpec streamSpec;
	streamSpec.m_functions = &gs_streamFuncs;
	streamSpec.m_userdata = f;
	streamSpec.m_permissions = 0;

	if (isReadable)
		streamSpec.m_permissions |= RkcStreamPermission_Read;

	if (isWriteable)
		streamSpec.m_permissions |= RkcStreamPermission_Write;

	return streamSpec;
}
