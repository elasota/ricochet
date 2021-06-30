#pragma once

#ifdef _MSC_VER
#include <sal.h>
#endif

#include <cstdint>
#include <cstddef>
#include <type_traits>

#if RKC_CONFIG_RELEASE
#	define RKC_IS_DEBUG 0
#else
#	define RKC_IS_DEBUG 1
#endif

#if __cplusplus >= 201703L
#	define RKC_IS_CPP17	1
#else
#	define RKC_IS_CPP17	0
#endif


#if RKC_IS_CPP17
#	define RKC_TYPE_NODISCARD [[nodiscard]]
#else
#	define RKC_TYPE_NODISCARD
#endif

#ifdef _MSC_VER
#	define RKC_IS_VISUAL_STUDIO	1
#else
#	define RKC_IS_VISUAL_STUDIO	0
#endif

#if defined(__clang__)
#	define RKC_IS_CLANG	1
#else
#	define RKC_IS_CLANG	0
#endif

#if defined(__GNUC__) || defined(__GNUG__)
#	define RKC_IS_GCC	1
#else
#	define RKC_IS_GCC	0
#endif

#if RKC_IS_GCC || RKC_IS_CLANG
#	define RKC_IS_CLANG_OR_GCC
#endif

#if RKC_IS_CLANG_OR_GCC
#	define RKC_WARN_UNUSED_RESULT_ATTRIB	__attribute__((warn_unused_result))
#else
#	define RKC_WARN_UNUSED_RESULT_ATTRIB _Must_inspect_result_
#endif

#if RKC_IS_DEBUG
#	include <cassert>
#	define RKC_ASSERT(n) assert(n)
#	define RKC_ASSERTS_ENABLED	1
#	ifdef NDEBUG
#		error "NDEBUG is defined in debug mode"
#	endif
#else
#	define RKC_ASSERT(n)
#	define RKC_ASSERTS_ENABLED	0
#endif

#define RKC_STATIC_ASSERT(n) static_assert((n), "Static assert condition (" #n ") not satisfied")

#define RKC_COMBINE_TOKENS(a, b) a##b
#define RKC_COMBINE_TOKENS2(a, b) RKC_COMBINE_TOKENS(a, b)

namespace rkci
{
	template<class T>
	struct RemoveReference
	{
		typedef T Type_t;
	};

	template<class T>
	struct RemoveReference<T&>
	{
		typedef T Type_t;
	};

	template<class T>
	struct RemoveReference<T&&>
	{
		typedef T Type_t;
	};

	template<class T>
	typename RemoveReference<T>::Type_t &&Move(T&& v) noexcept
	{
		return static_cast<typename RemoveReference<T>::Type_t&&>(v);
	}

	template<class T>
	typename RemoveReference<T>::Type_t &&Forward(T& v) noexcept
	{
		return static_cast<typename RemoveReference<T>::Type_t&&>(v);
	}

	typedef uint32_t Hash_t;
}
