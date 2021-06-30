#include "rkclib.h"
#include "Lexer.h"
#include "HashMap.h"
#include "MoveOrCopy.h"

struct RkcAllocator : public rkci::IAllocator
{
	explicit RkcAllocator(const RkcAllocatorSpec &allocatorSpec);

	void *Realloc(void *buf, size_t newSize) override;

	RkcAllocatorSpec m_allocSpec;
};

RkcAllocator::RkcAllocator(const RkcAllocatorSpec &allocatorSpec)
	: m_allocSpec(allocatorSpec)
{
}

void *RkcAllocator::Realloc(void *buf, size_t newSize)
{
	return m_allocSpec.m_realloc(m_allocSpec.m_userdata, buf, newSize);
}

struct RkcStream : public rkci::IStream
{
	explicit RkcStream(const RkcStreamSpec &streamSpec);

	 size_t Read(void *buf, size_t size) override;
	 size_t Write(void *buf, size_t size) override;
	 rkcUFilePos_t Tell() const override;
	 bool SeekStart(rkcUFilePos_t pos) override;
	 bool SeekEnd(rkcFilePos_t pos) override;
	 bool SeekCurrent(rkcFilePos_t pos) override;
	 bool IsReadable() const override;
	 bool IsWritable() const override;
	 void Close() override;

	 RkcStreamSpec m_streamSpec;
};

RkcStream::RkcStream(const RkcStreamSpec &streamSpec)
	: m_streamSpec(streamSpec)
{
}

size_t RkcStream::Read(void *buf, size_t size)
{
	return m_streamSpec.m_functions->m_read(m_streamSpec.m_userdata, buf, size);
}

size_t RkcStream::Write(void *buf, size_t size)
{
	return m_streamSpec.m_functions->m_write(m_streamSpec.m_userdata, buf, size);
}

rkcUFilePos_t RkcStream::Tell() const
{
	return m_streamSpec.m_functions->m_tell(m_streamSpec.m_userdata);
}

bool RkcStream::SeekStart(rkcUFilePos_t pos)
{
	return m_streamSpec.m_functions->m_seekStart(m_streamSpec.m_userdata, pos) != 0;
}

bool RkcStream::SeekEnd(rkcFilePos_t pos)
{
	return m_streamSpec.m_functions->m_seekEnd(m_streamSpec.m_userdata, pos) != 0;
}

bool RkcStream::SeekCurrent(rkcFilePos_t pos)
{
	return m_streamSpec.m_functions->m_seekCurrent(m_streamSpec.m_userdata, pos) != 0;
}

bool RkcStream::IsReadable() const
{
	return (m_streamSpec.m_permissions & RkcStreamPermission_Read) != 0;
}

bool RkcStream::IsWritable() const
{
	return (m_streamSpec.m_permissions & RkcStreamPermission_Write) != 0;
}

void RkcStream::Close()
{
	m_streamSpec.m_functions->m_close(m_streamSpec.m_userdata);
}


rkci::Result TestParseStreamInternal(rkci::IStream *stream, rkci::IAllocator *alloc)
{
	rkci::Lexer lexer(stream, alloc);

	for (;;)
	{
		RKC_CHECK_RV(rkci::LexToken, lexToken, lexer.GetNextToken());

		int n = 0;
	}

	return rkci::Result::Ok();
}



int RkcTestParseStream(const RkcStreamSpec *streamSpec, const RkcAllocatorSpec *allocSpec)
{
	RkcStream stream(*streamSpec);
	RkcAllocator allocator(*allocSpec);

	rkci::Result result(TestParseStreamInternal(&stream, &allocator));
	result.Handle();

	return result.GetCode();
}





int RkcRunTests(rkci::IAllocator &alloc);

void RkcTest(const RkcAllocatorSpec *allocSpec)
{
	RkcAllocator allocator(*allocSpec);

	RkcRunTests(allocator);
}
