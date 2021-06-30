#pragma once

#include "TypeTuple.h"

namespace rkci
{
	template<class... T> class Tuple;
	template<size_t TIndex, class... T> class TupleElement;

	// Zero-index TupleElement single
	template<class T>
	class TupleElement<0, T>
	{
	public:
		static T &Get(Tuple<T> &tuple);
		static const T &Get(const Tuple<T> &tuple);
	};

	// Zero-index TupleElement multiple
	template<class T, class... TRest>
	class TupleElement<0, T, TRest...>
	{
	public:
		static T &Get(Tuple<T, TRest...> &tuple);
		static const T &Get(const Tuple<T, TRest...> &tuple);
	};

	template<size_t TIndex, class T, class... TRest>
	class TupleElement<TIndex, T, TRest...> : public TupleElement<TIndex - 1, TRest...>
	{
	};

	// Single tuple
	template<class TThis>
	class Tuple<TThis>
	{
	public:
		Tuple();
		Tuple(TThis &&thisParam);

		TThis &GetFirst();
		const TThis &GetFirst() const;

		TThis &GetLast();
		const TThis &GetLast() const;

		template<size_t TIndex>
		TThis &Get();

		template<size_t TIndex>
		const TThis &Get() const;

	private:
		TThis m_value;
	};

	// Multi tuple
	template<class TThis, class... TRest>
	class Tuple<TThis, TRest...> : private Tuple<TRest...>
	{
	public:
		Tuple();
		Tuple(TThis &&thisParam, TRest&&... restParams);

		TThis &GetFirst();
		const TThis &GetFirst() const;

		typename TypeTuple<TThis, TRest...>::Last_t &GetLast();
		const typename TypeTuple<TThis, TRest...>::Last_t &GetLast() const;

		template<size_t TIndex>
		typename TypeTupleElement<TIndex, TThis, TRest...>::Type_t &Get();

		template<size_t TIndex>
		const typename TypeTupleElement<TIndex, TThis, TRest...>::Type_t &Get() const;

	private:
		TThis m_value;
	};
}

namespace rkci
{
	// Zero-index tuple element single
	template<class T>
	T &TupleElement<0, T>::Get(Tuple<T> &tuple)
	{
		return tuple.GetFirst();
	}

	template<class T>
	const T &TupleElement<0, T>::Get(const Tuple<T> &tuple)
	{
		return tuple.GetFirst();
	}

	// Zero-index tuple element multiple
	template<class T, class... TRest>
	T &TupleElement<0, T, TRest...>::Get(Tuple<T, TRest...> &tuple)
	{
		return tuple.GetFirst();
	}

	template<class T, class... TRest>
	const T &TupleElement<0, T, TRest...>::Get(const Tuple<T, TRest...> &tuple)
	{
		return tuple.GetFirst();
	}

	// Single tuple
	template<class TThis>
	Tuple<TThis>::Tuple()
		: m_value()
	{
	}

	template<class TThis>
	Tuple<TThis>::Tuple(TThis &&thisParam)
		: m_value(static_cast<TThis&&>(thisParam))
	{
	}

	template<class TThis>
	TThis &Tuple<TThis>::GetFirst()
	{
		return m_value;
	}

	template<class TThis>
	const TThis &Tuple<TThis>::GetFirst() const
	{
		return m_value;
	}

	template<class TThis>
	TThis &Tuple<TThis>::GetLast()
	{
		return m_value;
	}

	template<class TThis>
	const TThis &Tuple<TThis>::GetLast() const
	{
		return m_value;
	}


	template<class TThis>
	template<size_t TIndex>
	TThis &Tuple<TThis>::Get()
	{
		static_assert(TIndex == 0, "Index out of range");
		return m_value;
	}

	template<class TThis>
	template<size_t TIndex>
	const TThis &Tuple<TThis>::Get() const
	{
		static_assert(TIndex == 0, "Index out of range");
		return m_value;
	}


	// Multi tuple
	template<class TThis, class... TRest>
	Tuple<TThis, TRest...>::Tuple()
		: m_value()
	{
	}

	template<class TThis, class... TRest>
	Tuple<TThis, TRest...>::Tuple(TThis &&thisParam, TRest&&... restParams)
		: m_value(static_cast<TThis&&>(thisParam))
		, Tuple<TRest...>(static_cast<TRest&&...>(restParams))
	{
	}

	template<class TThis, class... TRest>
	TThis &Tuple<TThis, TRest...>::GetFirst()
	{
		return m_value;
	}

	template<class TThis, class... TRest>
	const TThis &Tuple<TThis, TRest...>::GetFirst() const
	{
		return m_value;
	}

	template<class TThis, class... TRest>
	typename TypeTuple<TThis, TRest...>::Last_t &Tuple<TThis, TRest...>::GetLast()
	{
		return Tuple<TRest...>::GetLast();
	}

	template<class TThis, class... TRest>
	const typename TypeTuple<TThis, TRest...>::Last_t &Tuple<TThis, TRest...>::GetLast() const
	{
		return Tuple<TRest...>::GetLast();
	}


	template<class TThis, class... TRest>
	template<size_t TIndex>
	typename TypeTupleElement<TIndex, TThis, TRest...>::Type_t &Tuple<TThis, TRest...>::Get()
	{
		return TupleElement<TIndex, TThis, TRest...>::Get(*this);
	}

	template<class TThis, class... TRest>
	template<size_t TIndex>
	const typename TypeTupleElement<TIndex, TThis, TRest...>::Type_t &Tuple<TThis, TRest...>::Get() const
	{
		return TupleElement<TIndex, TThis, TRest...>::Get(*this);
	}
}
