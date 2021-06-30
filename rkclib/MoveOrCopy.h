#pragma once

namespace rkci
{
	template<class T> class Cloner;
	template<class T> class ResultRV;
	class Result;

	template<class T>
	class MoveOrCopy
	{
	public:
		MoveOrCopy(const T &obj);
		MoveOrCopy(T &&obj);

		ResultRV<T> Produce() const;
		void Consume() const;
		const T &Get() const;

	private:
		struct IBehavior
		{
			virtual ResultRV<T> Produce(T &ref) const = 0;
			virtual void Consume(T &ref) const = 0;
		};

		class CloneBehavior final : public IBehavior
		{
		public:
			ResultRV<T> Produce(T &ref) const override;
			void Consume(T &ref) const override;

			static CloneBehavior ms_instance;
		};

		class MoveBehavior final : public IBehavior
		{
		public:
			ResultRV<T> Produce(T &ref) const override;
			void Consume(T &ref) const override;

			static MoveBehavior ms_instance;
		};

		T &m_ref;
		const IBehavior &m_behavior;
	};
}

#include "Cloner.h"
#include "Result.h"

template<class T>
rkci::MoveOrCopy<T>::MoveOrCopy(const T &obj)
	: m_ref(const_cast<T&>(obj))
	, m_behavior(CloneBehavior::ms_instance)
{
}

template<class T>
rkci::MoveOrCopy<T>::MoveOrCopy(T &&obj)
	: m_ref(obj)
	, m_behavior(MoveBehavior::ms_instance)
{
}

template<class T>
rkci::ResultRV<T> rkci::MoveOrCopy<T>::Produce() const
{
	return m_behavior.Produce(m_ref);
}

template<class T>
void rkci::MoveOrCopy<T>::Consume() const
{
	return m_behavior.Consume(m_ref);
}

template<class T>
const T &rkci::MoveOrCopy<T>::Get() const
{
	return m_ref;
}


template<class T>
rkci::ResultRV<T> rkci::MoveOrCopy<T>::CloneBehavior::Produce(T &ref) const
{
	RKC_CHECK_RV(T, clone, rkci::Cloner<T>::Clone(ref));
	return clone;
}

template<class T>
void rkci::MoveOrCopy<T>::CloneBehavior::Consume(T &ref) const
{
}

template<class T>
rkci::ResultRV<T> rkci::MoveOrCopy<T>::MoveBehavior::Produce(T &ref) const
{
	return T(static_cast<T&&>(ref));
}

template<class T>
void rkci::MoveOrCopy<T>::MoveBehavior::Consume(T &ref) const
{
	T discarded(static_cast<T&&>(ref));
	(void)discarded;
}

template<class T>
typename rkci::MoveOrCopy<T>::MoveBehavior rkci::MoveOrCopy<T>::MoveBehavior::ms_instance;

template<class T>
typename rkci::MoveOrCopy<T>::CloneBehavior rkci::MoveOrCopy<T>::CloneBehavior::ms_instance;
