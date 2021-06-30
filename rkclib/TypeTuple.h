#pragma once

namespace rkci
{
	template<class... T> class TypeTuple;
	template<size_t TIndex, class... T> class TypeTupleElement;

	template<>
	class TypeTuple<>
	{
	public:
		static const size_t kCount = 0;
	};

	template<class T>
	class TypeTuple<T>
	{
	public:
		static const size_t kCount = 1;
		typedef T First_t;
		typedef T Last_t;
	};

	template<class T, class... TRest>
	class TypeTuple<T, TRest...>
	{
	public:
		static const size_t kCount = TypeTuple<TRest...>::kCount + 1;

		typedef typename TypeTuple<TRest...>::Last_t Last_t;
		typedef T First_t;
	};

	template<class T>
	class TypeTupleElement<0, T>
	{
	public:
		typedef T Type_t;
	};

	template<class T, class... TRest>
	class TypeTupleElement<0, T, TRest...>
	{
	public:
		typedef T Type_t;
	};

	template<size_t TIndex, class T, class... TRest>
	class TypeTupleElement<TIndex, T, TRest...>
	{
	public:
		typedef typename TypeTupleElement<TIndex-1, TRest...>::Type_t Type_t;
	};
}
