#pragma once

#include "CoreDefs.h"
#include "ResultCode.h"

namespace rkci
{
	class RKC_TYPE_NODISCARD Result
	{
	public:
		Result(rkc::ResultCode_t resultCode);
		Result(Result&& other) noexcept;
		~Result();

		static Result Ok();

		bool IsOK() const;
		rkc::ResultCode_t GetCode() const;
		void Handle();

	private:
		Result();
		Result(const Result& other) = delete;
		rkc::ResultCode_t m_resultCode;
#if RKC_IS_DEBUG
		bool m_isHandled;

		static void Unhandled();
		static void AlreadyHandled();
#	if RKC_IS_DEBUG
		static void OnError();
#	endif
#endif
	};

	template<class T>
	class ResultRV
	{
	public:
		ResultRV(T &&rv);
		ResultRV(const T &rv);
		ResultRV(rkc::ResultCode_t resultCode);
		ResultRV(Result &&result);
		ResultRV(ResultRV<T> &&result);
		~ResultRV();

		bool IsOK() const;
		void Handle();

		Result &GetResult();
		const Result &GetResult() const;

		T &Get();
		const T &Get() const;

	private:
		ResultRV(const ResultRV &other) = delete;

		union ResultOrReturnValueUnion
		{
			Result m_result;
			T m_returnValue;

			explicit ResultOrReturnValueUnion(Result &&result);
			explicit ResultOrReturnValueUnion(T &&returnValue);
			explicit ResultOrReturnValueUnion(const T &returnValue);
			~ResultOrReturnValueUnion();
		};

		ResultOrReturnValueUnion m_u;
		bool m_hasRV;

#if RKC_IS_DEBUG
		bool m_isHandled;
#endif
	};
}

#define RKC_CHECK(n) do { ::rkci::Result rkc_check_##__LINE__(n); if (!rkc_check_##__LINE__.IsOK()) return rkc_check_##__LINE__; else rkc_check_##__LINE__.Handle(); } while(false)
#define RKC_CHECK_RV(type, name, n)	\
	::rkci::ResultRV<type> RKC_COMBINE_TOKENS2(rkc_check_, __LINE__)(n);\
	RKC_COMBINE_TOKENS2(rkc_check_, __LINE__).Handle(); \
	if (!RKC_COMBINE_TOKENS2(rkc_check_, __LINE__).IsOK())\
		return ::rkci::Result(static_cast<::rkci::Result&&>(RKC_COMBINE_TOKENS2(rkc_check_, __LINE__).GetResult()));\
	type name(static_cast<type&&>(RKC_COMBINE_TOKENS2(rkc_check_, __LINE__).Get()))

namespace rkci
{
	inline Result::Result()
		: m_resultCode(rkc::ResultCodes::kOK)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
	}

	inline Result::Result(rkc::ResultCode_t resultCode)
		: m_resultCode(resultCode)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
#if RKC_IS_DEBUG
		if (resultCode != rkc::ResultCodes::kOK)
			OnError();
#endif
	}

	inline Result::Result(Result&& other) noexcept
		: m_resultCode(other.m_resultCode)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
#if RKC_IS_DEBUG
		other.Handle();
#endif
	}

	inline Result::~Result()
	{
#if RKC_IS_DEBUG
		if (!m_isHandled)
			this->Unhandled();
#endif
	}

	inline bool Result::IsOK() const
	{
		return m_resultCode == rkc::ResultCodes::kOK;
	}

	inline rkc::ResultCode_t Result::GetCode() const
	{
		return m_resultCode;
	}

	inline Result Result::Ok()
	{
		return Result(::rkc::ResultCodes::kOK);
	}

	inline void rkci::Result::Handle()
	{
#if RKC_IS_DEBUG
		if (!m_isHandled)
			m_isHandled = true;
		else
			this->AlreadyHandled();
#endif
	}

	template<class T>
	ResultRV<T>::ResultRV(const T &rv)
		: m_u(rv)
		, m_hasRV(true)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
	}

	template<class T>
	ResultRV<T>::ResultRV(T &&rv)
		: m_u(static_cast<T&&>(rv))
		, m_hasRV(true)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
	}

	template<class T>
	ResultRV<T>::ResultRV(rkc::ResultCode_t resultCode)
		: m_u(Result(resultCode))
		, m_hasRV(false)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
		RKC_ASSERT(resultCode != ::rkc::ResultCodes::kOK);
	}

	template<class T>
	ResultRV<T>::ResultRV(Result &&result)
		: m_u(static_cast<Result&&>(result))
		, m_hasRV(false)
#if RKC_IS_DEBUG
		, m_isHandled(false)
#endif
	{
		RKC_ASSERT(result.GetCode() != ::rkc::ResultCodes::kOK);
	}

	template<class T>
	ResultRV<T>::ResultRV(ResultRV<T> &&result)
		: m_hasRV(result.m_hasRV)
#if RKC_IS_DEBUG
		, m_isHandled(result.m_isHandled)
#endif
	{
		m_u.~ResultOrValueUnion();
		if (result.m_hasRV)
			new (&m_u) ResultOrReturnValueUnion(static_cast<T&&>(result.m_u.m_returnValue));
		else
			new (&m_u) ResultOrReturnValueUnion(static_cast<Result&&>(result.m_u.m_result));
	}

	template<class T>
	ResultRV<T>::~ResultRV()
	{
#if RKC_IS_DEBUG
		RKC_ASSERT(m_isHandled);
#endif

		if (m_hasRV)
			m_u.m_returnValue.~T();
		else
			m_u.m_result.~Result();
	}

	template<class T>
	bool ResultRV<T>::IsOK() const
	{
		return m_hasRV;
	}

	template<class T>
	void ResultRV<T>::Handle()
	{
		RKC_ASSERT(!m_isHandled);
		m_isHandled = true;
	}

	template<class T>
	Result &ResultRV<T>::GetResult()
	{
		RKC_ASSERT(!m_hasRV);
		return m_u.m_result;
	}

	template<class T>
	const Result &ResultRV<T>::GetResult() const
	{
		RKC_ASSERT(!m_hasRV);
		return m_u.m_result;
	}

	template<class T>
	T &ResultRV<T>::Get()
	{
		RKC_ASSERT(m_hasRV);
		return m_u.m_returnValue;
	}

	template<class T>
	const T &ResultRV<T>::Get() const
	{
		RKC_ASSERT(m_hasRV);
		return m_u.m_returnValue;
	}

	template<class T>
	ResultRV<T>::ResultOrReturnValueUnion::ResultOrReturnValueUnion(Result &&result)
		: m_result(static_cast<Result&&>(result))
	{
	}

	template<class T>
	ResultRV<T>::ResultOrReturnValueUnion::ResultOrReturnValueUnion(const T &returnValue)
		: m_returnValue(returnValue)
	{
	}

	template<class T>
	ResultRV<T>::ResultOrReturnValueUnion::ResultOrReturnValueUnion(T &&returnValue)
		: m_returnValue(static_cast<T&&>(returnValue))
	{
	}

	template<class T>
	ResultRV<T>::ResultOrReturnValueUnion::~ResultOrReturnValueUnion()
	{
	}
}
