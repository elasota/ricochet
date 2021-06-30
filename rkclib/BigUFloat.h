#pragma once

#include "CoreDefs.h"
#include "Vector.h"

namespace rkci
{
	struct IAllocator;
	class Result;
	template<class T> class ResultRV;
	template<class T> class Cloner;

	struct DecBin;
	struct NumUtils;

	/*
	Properties struct members:
	struct Properties
	{
		typedef TFragment Fragment_t;
		typedef TDoubleFragment DoubleFragment_t;

		static const Fragment_t kDigitModulo = TDigitModulo;
		static const Fragment_t kFragmentModulo = TFragmentModulo;
		static const uint8_t kDigitsPerFragment = TDigitsPerFragment;
		static const unsigned int kNumStaticFragments = TNumStaticFragments;

		static const size_t kMaxDigits = TMaxDigits;
		static const int32_t kMaxLowPlace = TMaxLowPlace;
		static const int32_t kMinLowPlace = -TMinLowPlace;

		Fragment_t GetFragmentPower(uint32_t power);
	};
	*/

	template<class T>
	class BigUFloat
	{
	public:
		friend struct DecBin;
		friend struct NumUtils;

		typedef typename T::Fragment_t Fragment_t;
		typedef typename T::FragmentWithCarry_t FragmentWithCarry_t;
		typedef typename T::DoubleFragment_t DoubleFragment_t;
		typedef T Properties_t;

		static const uint8_t kBase = T::kBase;
		static const FragmentWithCarry_t kFragmentModulo = T::kFragmentModulo;
		static const uint8_t kDigitsPerFragment = T::kDigitsPerFragment;
		static const unsigned int kNumStaticFragments = T::kNumStaticFragments;

		static const size_t kMaxDigits = T::kMaxDigits;
		static const int32_t kMaxLowPlace = T::kMaxLowPlace;
		static const int32_t kMinLowPlace = T::kMinLowPlace;

		typedef Vector<Fragment_t, kNumStaticFragments> FragmentVector_t;

		BigUFloat();
		BigUFloat(Fragment_t initialFragment, IAllocator &alloc);
		BigUFloat(BigUFloat &&other);
		~BigUFloat();

		Fragment_t GetFragment(size_t offset) const;
		int32_t GetLowPlace() const;
		uint32_t GetNumDigits() const;
		uint32_t GetNumFragments() const;
		IAllocator *GetAllocator() const;
		bool IsZero() const;

		Result ShiftInPlace(int32_t offset);
		Result AddInPlace(const BigUFloat<T> &other);
		Result SubtractInPlace(const BigUFloat<T> &other);
		Result MultiplyInPlace(const BigUFloat<T> &other);

		ResultRV<BigUFloat<T>> Clone() const;

		BigUFloat<T> &operator=(BigUFloat<T> &&other);

		bool operator<(const BigUFloat<T> &other) const;
		bool operator<=(const BigUFloat<T> &other) const;
		bool operator>(const BigUFloat<T> &other) const;
		bool operator>=(const BigUFloat<T> &other) const;
		bool operator==(const BigUFloat<T> &other) const;
		bool operator!=(const BigUFloat<T> &other) const;

	private:
		BigUFloat<T>(int32_t lowPlace, uint32_t numDigits, FragmentVector_t &&fragments);
		BigUFloat<T>(const BigUFloat<T> &other) = delete;
		BigUFloat<T> &operator=(const BigUFloat<T> &other) = delete;

		Result AssignAddInPlaceSorted(const BigUFloat<T> &lower, const BigUFloat<T> &higher);
		bool CompareFirstMismatchedFragment(const BigUFloat<T> &other, bool(*func)(const Fragment_t &a, const Fragment_t &b)) const;

		static Result NormalizeFragments(FragmentVector_t &fragVector, uint32_t &outRemovedLowDigits, uint32_t &outSignificantDigits);

		int32_t m_lowPlace;
		uint32_t m_numDigits;
		FragmentVector_t m_fragments;
	};

	template<class T>
	class Cloner<BigUFloat<T>>
	{
	public:
		static rkci::ResultRV<BigUFloat<T>> Clone(const BigUFloat<T> &t);
	};
}

template<class T>
inline rkci::BigUFloat<T>::BigUFloat()
	: m_lowPlace(0)
	, m_numDigits(0)
	, m_fragments(nullptr)
{
}



#include "IAllocator.h"
#include "Result.h"
#include "Comparer.h"

#include <algorithm>

template<class T>
rkci::BigUFloat<T>::BigUFloat(BigUFloat<T> &&other)
	: m_lowPlace(other.m_lowPlace)
	, m_numDigits(other.m_numDigits)
	, m_fragments(rkci::Move(other.m_fragments))
{
	other.m_lowPlace = 0;
	other.m_numDigits = 0;
}

template<class T>
rkci::BigUFloat<T>::BigUFloat(int32_t lowPlace, uint32_t numDigits, FragmentVector_t &&fragments)
	: m_lowPlace(lowPlace)
	, m_numDigits(numDigits)
	, m_fragments(rkci::Move(fragments))
{
}

template<class T>
rkci::BigUFloat<T>::BigUFloat(Fragment_t initialFragment, IAllocator &alloc)
	: m_lowPlace(0)
	, m_numDigits(0)
	, m_fragments(&alloc)
{
	RKC_ASSERT(static_cast<FragmentWithCarry_t>(initialFragment) < kFragmentModulo);

	if (initialFragment == 0)
		return;

	m_lowPlace = T::CountTrailingZeroes(initialFragment);
	initialFragment /= T::GetFragmentPower(m_lowPlace);

	while (m_numDigits < kDigitsPerFragment && initialFragment >= T::GetFragmentPower(m_numDigits))
		m_numDigits++;

	m_fragments.ResizeStatic(1);
	m_fragments[0] = initialFragment;
}

template<class T>
rkci::BigUFloat<T>::~BigUFloat()
{
}

template<class T>
typename rkci::BigUFloat<T>::Fragment_t rkci::BigUFloat<T>::GetFragment(size_t index) const
{
	return m_fragments[index];
}

template<class T>
int32_t rkci::BigUFloat<T>::GetLowPlace() const
{
	return m_lowPlace;
}

template<class T>
uint32_t rkci::BigUFloat<T>::GetNumDigits() const
{
	return m_numDigits;
}

template<class T>
uint32_t rkci::BigUFloat<T>::GetNumFragments() const
{
	return static_cast<uint32_t>(m_fragments.Count());
}

template<class T>
rkci::IAllocator *rkci::BigUFloat<T>::GetAllocator() const
{
	return m_fragments.GetAllocator();
}

template<class T>
bool rkci::BigUFloat<T>::IsZero() const
{
	return m_numDigits == 0;
}

template<class T>
rkci::Result rkci::BigUFloat<T>::ShiftInPlace(int32_t offset)
{
	if (m_numDigits == 0)
		return Result::Ok();

	const int32_t kRange = kMaxLowPlace - kMinLowPlace;
	if (offset < -kRange || offset > kRange)
		return rkc::ResultCodes::kIntegerOverflow;

	const int32_t newLowPlace = m_lowPlace + offset;
	if (newLowPlace >= -kMinLowPlace || newLowPlace >= kMaxLowPlace)
		return rkc::ResultCodes::kIntegerOverflow;

	m_lowPlace = newLowPlace;

	return Result::Ok();
}

template<class T>
rkci::Result rkci::BigUFloat<T>::AddInPlace(const BigUFloat<T> &other)
{
	if (other.IsZero())
		return rkci::Result::Ok();

	if (this->IsZero())
	{
		RKC_CHECK_RV(BigUFloat<T>, clone, other.Clone());
		(*this) = rkci::Move(clone);
		return rkci::Result::Ok();
	}

	if (m_lowPlace < other.m_lowPlace)
		return AssignAddInPlaceSorted(*this, other);
	else
		return AssignAddInPlaceSorted(other, *this);
}



template<class T>
rkci::Result rkci::BigUFloat<T>::SubtractInPlace(const BigUFloat<T> &other)
{
	if (other.IsZero())
		return Result::Ok();

	if (other == (*this))
	{
		(*this) = BigUFloat<T>();
		return Result::Ok();
	}

	RKC_ASSERT(other < *this);

	const int32_t otherLowPlaceRelativeToThisFragment = other.GetLowPlace() - m_lowPlace;
	uint32_t fragmentsBelowThis = 0;

	FragmentVector_t newFrags(m_fragments.GetAllocator());
	uint32_t fragmentMisalignment = 0;
	if (otherLowPlaceRelativeToThisFragment < 0)
	{
		const int32_t lowerFragments = ((-otherLowPlaceRelativeToThisFragment) + kDigitsPerFragment - 1) / kDigitsPerFragment;
		fragmentsBelowThis = static_cast<uint32_t>(lowerFragments);
		fragmentMisalignment = (-otherLowPlaceRelativeToThisFragment) % kDigitsPerFragment;
		if (fragmentMisalignment != 0)
			fragmentMisalignment = kDigitsPerFragment - fragmentMisalignment;
	}
	else
		fragmentMisalignment = static_cast<uint32_t>(otherLowPlaceRelativeToThisFragment % kDigitsPerFragment);

	size_t thisFragmentsCount = m_fragments.Count();
	size_t numResultFragments = fragmentsBelowThis + thisFragmentsCount;
	RKC_CHECK(newFrags.Resize(numResultFragments));

	const int32_t otherLowPlaceDifference = other.GetLowPlace() - m_lowPlace;
	ArraySliceView<const Fragment_t> thisFragmentsSlice = m_fragments.Slice();
	ArraySliceView<const Fragment_t> otherFragmentsSlice = other.m_fragments.Slice();
	ArraySliceView<Fragment_t> newFragmentsSlice = newFrags.Slice();

	if (fragmentMisalignment != 0)
	{
		const Fragment_t splitModulo = T::GetFragmentPower(kDigitsPerFragment - fragmentMisalignment);
		const Fragment_t raiseModulo = T::GetFragmentPower(fragmentMisalignment);

		Fragment_t fillFragment = 0;
		Fragment_t nextFragmentLow = 0;
		for (size_t fragIndex = 0; fragIndex < numResultFragments; fragIndex++)
		{
			int32_t placeInCurrent = (static_cast<int32_t>(fragIndex) - static_cast<int32_t>(fragmentsBelowThis)) * kDigitsPerFragment;
			int32_t placeInOther = placeInCurrent  - otherLowPlaceDifference;

			fillFragment = nextFragmentLow;
			nextFragmentLow = 0;
			if (placeInOther + kDigitsPerFragment >= 0)
			{
				uint32_t otherFragmentIndex = static_cast<uint32_t>(placeInOther + kDigitsPerFragment) / kDigitsPerFragment;

				if (otherFragmentIndex < otherFragmentsSlice.Count())
				{
					const Fragment_t otherFragment = otherFragmentsSlice[otherFragmentIndex];
					const Fragment_t otherFragmentLow = otherFragment % splitModulo;
					const Fragment_t otherFragmentHigh = otherFragment / splitModulo;

					nextFragmentLow = otherFragmentHigh;
					fillFragment += otherFragmentLow * raiseModulo;
				}
			}

			newFragmentsSlice[fragIndex] = fillFragment;
		}
	}
	else
	{
		const int32_t otherFragIndexToNewFragIndex = otherLowPlaceRelativeToThisFragment / kDigitsPerFragment + static_cast<int32_t>(fragmentsBelowThis);

		for (size_t otherFragIndex = 0; otherFragIndex < otherFragmentsSlice.Count(); otherFragIndex++)
			newFragmentsSlice[static_cast<size_t>(static_cast<int32_t>(otherFragIndex) + otherFragIndexToNewFragIndex)] = otherFragmentsSlice[otherFragIndex];
	}

	// Invert other fragments
	for (size_t i = 0; i < numResultFragments; i++)
		newFragmentsSlice[i] = static_cast<Fragment_t>(kFragmentModulo - 1) - newFragmentsSlice[i];

	// Add this fragments
	{
		bool carry = true;
		for (size_t subFragIndex = 0; subFragIndex < fragmentsBelowThis; subFragIndex++)
		{
			if (newFragmentsSlice[subFragIndex] == static_cast<Fragment_t>(kFragmentModulo - 1))
				newFragmentsSlice[subFragIndex] = 0;
			else
			{
				newFragmentsSlice[subFragIndex]++;
				carry = false;
				break;
			}
		}

		for (size_t thisFragIndex = 0; thisFragIndex < thisFragmentsCount; thisFragIndex++)
		{
			size_t resultFragIndex = thisFragIndex + fragmentsBelowThis;
			FragmentWithCarry_t added = newFragmentsSlice[resultFragIndex] + thisFragmentsSlice[thisFragIndex];
			if (carry)
				added++;

			carry = (added >= kFragmentModulo);
			if (carry)
				added -= kFragmentModulo;

			newFragmentsSlice[resultFragIndex] = static_cast<Fragment_t>(added);
		}
	}

	uint32_t removedLowDigits = 0;
	uint32_t significantDigits = 0;
	RKC_CHECK(NormalizeFragments(newFrags, removedLowDigits, significantDigits));

	const int32_t lowPlace = m_lowPlace - static_cast<int32_t>(fragmentsBelowThis * kDigitsPerFragment) + removedLowDigits;

	(*this) = BigUFloat<T>(lowPlace, significantDigits, rkci::Move(newFrags));

	return Result::Ok();
}

template<class T>
rkci::Result rkci::BigUFloat<T>::MultiplyInPlace(const BigUFloat<T> &other)
{
	if (other.IsZero() || this->IsZero())
		return rkci::Result::Ok();

	if (this->GetNumDigits() == 1)
	{
		const Fragment_t fragment = this->GetFragment(0);
		if (fragment == 1)
		{
			RKC_CHECK_RV(BigUFloat<T>, clone, other.Clone());
			RKC_CHECK(clone.ShiftInPlace(this->m_lowPlace));
			(*this) = rkci::Move(clone);
			return Result::Ok();
		}
		if (fragment == 2)
		{
			RKC_CHECK_RV(BigUFloat<T>, clone, other.Clone());
			RKC_CHECK(clone.AddInPlace(clone));
			RKC_CHECK(clone.ShiftInPlace(this->m_lowPlace));
			(*this) = rkci::Move(clone);
			return Result::Ok();
		}
	}

	if (other.GetNumDigits() == 1)
	{
		const Fragment_t fragment = other.GetFragment(0);
		if (fragment == 1)
		{
			RKC_CHECK_RV(BigUFloat<T>, clone, this->Clone());
			RKC_CHECK(clone.ShiftInPlace(other.GetLowPlace()));
			(*this) = rkci::Move(clone);
			return Result::Ok();
		}
		if (fragment == 2)
		{
			RKC_CHECK_RV(BigUFloat<T>, clone, this->Clone());
			RKC_CHECK(clone.AddInPlace(clone));
			RKC_CHECK(clone.ShiftInPlace(other.GetLowPlace()));
			(*this) = rkci::Move(clone);
			return Result::Ok();
		}
	}

	FragmentVector_t fragVector(m_fragments.GetAllocator());
	uint32_t maxSignificantDigits = this->m_numDigits + other.m_numDigits + 1;
	size_t maxFragments = (maxSignificantDigits + kDigitsPerFragment - 1) / kDigitsPerFragment;

	RKC_CHECK(fragVector.Resize(maxFragments));
	size_t thisNumFragments = m_fragments.Count();
	size_t otherNumFragments = other.m_fragments.Count();

	ArraySliceView<const Fragment_t> thisFrags = m_fragments.Slice();
	ArraySliceView<const Fragment_t> otherFrags = other.m_fragments.Slice();
	ArraySliceView<Fragment_t> newFrags = fragVector.Slice();

	for (size_t i = 0; i < maxFragments; i++)
		newFrags[i] = 0;

	for (size_t otherFragIndex = 0; otherFragIndex < otherNumFragments; otherFragIndex++)
	{
		const Fragment_t otherFragment = otherFrags[otherFragIndex];

		Fragment_t mulCarry = 0;
		for (size_t thisFragIndex = 0; thisFragIndex <= thisNumFragments; thisFragIndex++)
		{
			if (thisFragIndex + otherFragIndex == maxFragments)
				break;

			Fragment_t fragmentToInsert = 0;
			if (thisFragIndex == thisNumFragments)
				fragmentToInsert = mulCarry;
			else
			{
				const Fragment_t thisFragment = thisFrags[thisFragIndex];
				const DoubleFragment_t multiplied = static_cast<DoubleFragment_t>(thisFragment) * static_cast<DoubleFragment_t>(otherFragment) + mulCarry;

				const Fragment_t mulHighFragment = static_cast<Fragment_t>(multiplied / kFragmentModulo);
				const Fragment_t mulLowFragment = static_cast<Fragment_t>(multiplied % kFragmentModulo);

				mulCarry = mulHighFragment;
				fragmentToInsert = mulLowFragment;
			}

			size_t insertPos = thisFragIndex + otherFragIndex;
			const FragmentWithCarry_t addedInFragment = static_cast<FragmentWithCarry_t>(newFrags[thisFragIndex + otherFragIndex]) + fragmentToInsert;
			newFrags[insertPos] = static_cast<Fragment_t>(addedInFragment % kFragmentModulo);

			if (addedInFragment >= kFragmentModulo)
			{
				RKC_ASSERT(addedInFragment / kFragmentModulo == 1);
				for (;;)
				{
					insertPos++;
					const Fragment_t preCarryFrag = newFrags[insertPos];
					if (preCarryFrag == kFragmentModulo - 1)
					{
						// Carry
						newFrags[insertPos] = 0;
					}
					else
					{
						// Terminate
						newFrags[insertPos] = preCarryFrag + 1;
						break;
					}
				}
			}
		}
	}

	uint32_t removedLowDigits = 0;
	uint32_t significantDigits = 0;
	RKC_CHECK(NormalizeFragments(fragVector, removedLowDigits, significantDigits));

	const int32_t newLowPlace = m_lowPlace + other.m_lowPlace + static_cast<int32_t>(removedLowDigits);
	if (newLowPlace < kMinLowPlace || newLowPlace > kMaxLowPlace)
		return rkc::ResultCodes::kIntegerOverflow;

	m_lowPlace = newLowPlace;
	m_numDigits = significantDigits;
	m_fragments = rkci::Move(fragVector);

	return Result::Ok();
}

template<class T>
rkci::Result rkci::BigUFloat<T>::AssignAddInPlaceSorted(const rkci::BigUFloat<T> &lower, const rkci::BigUFloat<T> &higher)
{
	const int32_t lowPlaceInclusive = lower.m_lowPlace;
	RKC_ASSERT(higher.m_lowPlace >= lowPlaceInclusive);

	const int32_t highPlaceExclusive = std::max<int32_t>(lower.m_lowPlace + lower.m_numDigits, higher.m_lowPlace + higher.m_numDigits);
	RKC_ASSERT(highPlaceExclusive >= lowPlaceInclusive);

	// +1 for carry digit
	const size_t maxRequiredDigits = static_cast<size_t>(highPlaceExclusive - lowPlaceInclusive) + 1;
	const size_t maxRequiredFragments = (maxRequiredDigits + kDigitsPerFragment - 1) / kDigitsPerFragment;

	FragmentVector_t added(m_fragments.GetAllocator());
	RKC_CHECK(added.Resize(maxRequiredFragments));

	const int32_t highDistanceAboveLow = higher.m_lowPlace - lower.m_lowPlace;
	const int32_t highFragmentModuloPower = kDigitsPerFragment - (highDistanceAboveLow % kDigitsPerFragment);
	const int32_t highFragmentUpshiftPower = highDistanceAboveLow % kDigitsPerFragment;

	const Fragment_t highFragmentModulo = (highFragmentUpshiftPower == 0) ? 0 : T::GetFragmentPower(highFragmentModuloPower);
	const Fragment_t highFragmentUpshift = (highFragmentUpshiftPower == 0) ? 0 : T::GetFragmentPower(highFragmentUpshiftPower);

	size_t currentLowerFragment = 0;
	int32_t higherCurrentPlace = lower.m_lowPlace - higher.m_lowPlace;

	const ArraySliceView<const Fragment_t> lowerFragments = lower.m_fragments.Slice();
	const ArraySliceView<const Fragment_t> higherFragments = higher.m_fragments.Slice();

	const size_t numLowerFragments = lower.m_fragments.Count();
	const size_t numHigherFragments = higher.m_fragments.Count();

	bool carry = false;
	Fragment_t hLastSlicedFragmentUpper = 0;

	size_t currentHigherFragment = 0;

	for (size_t currentFragment = 0; currentFragment < maxRequiredFragments; currentFragment++)
	{
		FragmentWithCarry_t fragmentResult = carry ? 1 : 0;
		carry = false;

		if (currentFragment < numLowerFragments)
			fragmentResult += lowerFragments[currentFragment];

		if (higherCurrentPlace + kDigitsPerFragment > 0)
		{
			if (highFragmentUpshiftPower == 0)
			{
				if (currentHigherFragment < numHigherFragments)
					fragmentResult += higherFragments[currentHigherFragment];
			}
			else
			{
				fragmentResult += hLastSlicedFragmentUpper;
				if (currentHigherFragment < numHigherFragments)
				{
					const Fragment_t higherFragmentSrc = higherFragments[currentHigherFragment];
					const Fragment_t higherFragmentUpper = higherFragmentSrc / highFragmentModulo;
					const Fragment_t higherFragmentLower = higherFragmentSrc % highFragmentModulo;
					fragmentResult += higherFragmentLower * highFragmentUpshift;
					hLastSlicedFragmentUpper = higherFragmentUpper;
				}
				else
					hLastSlicedFragmentUpper = 0;
			}

			currentHigherFragment++;
		}

		if (fragmentResult >= kFragmentModulo)
		{
			carry = true;
			fragmentResult -= kFragmentModulo;
			RKC_ASSERT(fragmentResult < kFragmentModulo);
		}

		added[currentFragment] = static_cast<Fragment_t>(fragmentResult);

		higherCurrentPlace += kDigitsPerFragment;
	}

	RKC_ASSERT(carry == false);

	// Determine MSD position, which is either maxRequiredDigits or maxRequiredDigits-1
	size_t msdPositionExclusiveRelativeToLowPosition = maxRequiredDigits;
	{
		const size_t carryPositionFromLow = maxRequiredDigits - 1;
		const size_t carryPositionInFragment = carryPositionFromLow % kDigitsPerFragment;
		const size_t carryPositionFragment = carryPositionFromLow / kDigitsPerFragment;

		if (added[carryPositionFragment] < T::GetFragmentPower(carryPositionInFragment))
			msdPositionExclusiveRelativeToLowPosition--;
	}

	size_t lsdPositionInFragment = 0;
	size_t lsdFragment = 0;
	for (size_t fragIndex = 0; fragIndex < maxRequiredFragments; fragIndex++)
	{
		const Fragment_t fragmentValue = added[fragIndex];
		if (fragmentValue == 0)
			lsdFragment++;
		else
		{
			lsdPositionInFragment = T::CountTrailingZeroes(fragmentValue);
			break;
		}
	}

	size_t lsdPositionInclusiveRelativeToLowPosition = lsdFragment * kDigitsPerFragment + lsdPositionInFragment;
	if (lsdPositionInFragment > 0)
	{
		const Fragment_t shiftDownModulo = T::GetFragmentPower(lsdPositionInFragment);
		const Fragment_t shiftUpModulo = T::GetFragmentPower(kDigitsPerFragment - lsdPositionInFragment);

		added[lsdFragment] /= shiftDownModulo;
		for (size_t fragIndex = lsdFragment + 1; fragIndex < maxRequiredFragments; fragIndex++)
		{
			const Fragment_t fragToMoveDown = added[fragIndex];
			const Fragment_t upperFrag = fragToMoveDown / shiftDownModulo;
			const Fragment_t lowerFrag = fragToMoveDown % shiftDownModulo;

			added[fragIndex - 1] += lowerFrag * shiftUpModulo;
			added[fragIndex] = upperFrag;
		}
	}

	if (lsdFragment != 0)
	{
		for (size_t fragIndex = lsdFragment; fragIndex < maxRequiredFragments; fragIndex++)
			added[fragIndex - lsdFragment] = added[fragIndex];
	}

	const size_t numRealDigits = msdPositionExclusiveRelativeToLowPosition - lsdPositionInclusiveRelativeToLowPosition;
	const size_t numRealFragments = (numRealDigits + kDigitsPerFragment - 1) / kDigitsPerFragment;

	RKC_CHECK(added.Resize(numRealFragments));

	if (numRealFragments <= kNumStaticFragments)
		added.Optimize();

	m_lowPlace = lower.m_lowPlace + static_cast<int32_t>(lsdPositionInclusiveRelativeToLowPosition);
	m_numDigits = static_cast<uint32_t>(numRealDigits);
	m_fragments = rkci::Move(added);

	return Result::Ok();
}

template<class T>
bool rkci::BigUFloat<T>::CompareFirstMismatchedFragment(const BigUFloat<T> &other, bool(*func)(const Fragment_t &a, const Fragment_t &b)) const
{
	RKC_ASSERT(static_cast<int32_t>(m_numDigits) + m_lowPlace == static_cast<int32_t>(other.m_numDigits) + other.m_lowPlace);

	int32_t otherPlaceToThisPlace = m_lowPlace - other.m_lowPlace;
	int32_t thisPlaceToOtherPlace = other.m_lowPlace - m_lowPlace;

	const ArraySliceView<const Fragment_t> thisFrags = m_fragments.Slice();
	const ArraySliceView<const Fragment_t> otherFrags = other.m_fragments.Slice();

	int32_t digitMisalignment = otherPlaceToThisPlace % static_cast<int32_t>(kDigitsPerFragment);
	int32_t fragmentMisalignment = 0;
	if (digitMisalignment < 0)
		digitMisalignment += kDigitsPerFragment;

	fragmentMisalignment = (otherPlaceToThisPlace - digitMisalignment) / kDigitsPerFragment;

	// digit + fragment misalignment is how far RIGHT other needs to be shifted to align with our fragments

	if (digitMisalignment == 0)
	{
		for (size_t rfi = 0; rfi < thisFrags.Count(); rfi++)
		{
			const size_t fragmentIndex = thisFrags.Count() - 1 - rfi;
			const int32_t otherFragmentIndex = static_cast<int32_t>(fragmentIndex) + fragmentMisalignment;

			Fragment_t otherFrag = 0;
			const Fragment_t thisFrag = thisFrags[otherFragmentIndex];
			if (otherFragmentIndex < 0 || static_cast<size_t>(otherFragmentIndex) >= otherFrags.Count())
				otherFrag = otherFrags[otherFragmentIndex];
		}

		if (fragmentMisalignment > 0)
		{
			for (size_t rfi = 0; rfi < static_cast<size_t>(fragmentMisalignment); rfi++)
			{
				const size_t otherSplittableFragmentIndex = static_cast<size_t>(fragmentMisalignment - 1) - rfi;

				const Fragment_t otherFragment = otherFrags[otherSplittableFragmentIndex];

				if (otherFragment != 0)
					return func(0, otherFragment);
			}
		}

		return false;
	}
	else
	{
		const Fragment_t split = T::GetFragmentPower(digitMisalignment);
		const Fragment_t raise = T::GetFragmentPower(kDigitsPerFragment - digitMisalignment);

		const int32_t otherFragmentWithHighHalf = static_cast<int32_t>(thisFrags.Count()) + fragmentMisalignment;
		Fragment_t nextOtherFragment = 0;
		if (otherFragmentWithHighHalf >= 0 && static_cast<size_t>(otherFragmentWithHighHalf) < otherFrags.Count())
			nextOtherFragment = otherFrags[otherFragmentWithHighHalf] % split * raise;

		for (size_t rfi = 0; rfi < thisFrags.Count(); rfi++)
		{
			const size_t fragmentIndex = thisFrags.Count() - 1 - rfi;
			const int32_t otherSplittableFragmentIndex = static_cast<int32_t>(fragmentIndex) + fragmentMisalignment;

			Fragment_t otherFragment = nextOtherFragment;
			nextOtherFragment = 0;


			if (otherSplittableFragmentIndex >= 0 && static_cast<size_t>(otherSplittableFragmentIndex) < otherFrags.Count())
			{
				const Fragment_t otherFragmentUnsplit = otherFrags[otherSplittableFragmentIndex];
				otherFragment += otherFragmentUnsplit / split;
				nextOtherFragment = otherFragmentUnsplit % split * raise;
			}

			const Fragment_t thisFragment = thisFrags[fragmentIndex];
			if (thisFragment != otherFragment)
				return func(thisFragment, otherFragment);
		}

		if (fragmentMisalignment > 0)
		{
			for (size_t rfi = 0; rfi < static_cast<size_t>(fragmentMisalignment); rfi++)
			{
				const size_t otherSplittableFragmentIndex = static_cast<size_t>(fragmentMisalignment - 1) - rfi;

				const Fragment_t otherFragmentUnsplit = otherFrags[otherSplittableFragmentIndex];
				const Fragment_t otherFragment = nextOtherFragment + otherFragmentUnsplit / split;
				nextOtherFragment = otherFragmentUnsplit % split * raise;
			}

			if (nextOtherFragment != 0)
				return func(0, nextOtherFragment);
		}

		if (nextOtherFragment != 0)
			return func(0, nextOtherFragment);

		return false;
	}
}

template<class T>
rkci::Result rkci::BigUFloat<T>::NormalizeFragments(FragmentVector_t &fragVector, uint32_t &outRemovedLowDigits, uint32_t &outSignificantDigits)
{
	ArraySliceView<Fragment_t> slice = fragVector.Slice();

	uint32_t numDroppedLowerFragments = 0;
	while (slice[numDroppedLowerFragments] == 0)
		numDroppedLowerFragments++;

	uint32_t numDroppedLowZeroes = T::CountTrailingZeroes(slice[numDroppedLowerFragments]);

	if (numDroppedLowZeroes != 0)
	{
		const Fragment_t splitDivisor = T::GetFragmentPower(numDroppedLowZeroes);
		const Fragment_t lowerToUpperShift = T::GetFragmentPower(kDigitsPerFragment - numDroppedLowZeroes);
		slice[numDroppedLowerFragments] = slice[numDroppedLowerFragments] / splitDivisor;

		for (size_t srcFragIndex = numDroppedLowerFragments + 1; srcFragIndex < slice.Count(); srcFragIndex++)
		{
			const Fragment_t srcFrag = slice[srcFragIndex];
			const Fragment_t lowerPart = srcFrag % splitDivisor;
			const Fragment_t upperPart = srcFrag / splitDivisor;

			slice[srcFragIndex - 1] += lowerPart * lowerToUpperShift;
			slice[srcFragIndex] = upperPart;
		}
	}

	size_t numUpperFragmentsToRemove = 0;
	while (slice[slice.Count() - 1 - numUpperFragmentsToRemove] == 0)
		numUpperFragmentsToRemove++;

	const size_t numFragmentsToKeep = slice.Count() - numUpperFragmentsToRemove - numDroppedLowerFragments;

	if (numDroppedLowerFragments > 0)
		for (size_t i = 0; i < numFragmentsToKeep; i++)
			slice[i] = slice[numDroppedLowerFragments + i];

	const Fragment_t lastFragment = slice[numFragmentsToKeep - 1];
	uint32_t topFragmentDigits = 1;
	while (topFragmentDigits < kDigitsPerFragment && lastFragment >= T::GetFragmentPower(topFragmentDigits))
		topFragmentDigits++;

	RKC_CHECK(fragVector.Resize(numFragmentsToKeep));
	fragVector.Optimize();

	outRemovedLowDigits = numDroppedLowerFragments * kDigitsPerFragment + numDroppedLowZeroes;
	outSignificantDigits = static_cast<uint32_t>((numFragmentsToKeep - 1) * kDigitsPerFragment + topFragmentDigits);

	return Result::Ok();
}

template<class T>
rkci::ResultRV<rkci::BigUFloat<T>> rkci::BigUFloat<T>::Clone() const
{
	RKC_CHECK_RV(FragmentVector_t, fragmentClone, m_fragments.Clone());
	return BigUFloat<T>(m_lowPlace, m_numDigits, rkci::Move(fragmentClone));
}

template<class T>
rkci::BigUFloat<T> &rkci::BigUFloat<T>::operator=(BigUFloat<T> &&other)
{
	if (this == &other)
		return *this;

	m_lowPlace = other.m_lowPlace;
	m_numDigits = other.m_numDigits;
	m_fragments = rkci::Move(other.m_fragments);

	other.m_lowPlace = 0;
	other.m_numDigits = 0;

	return *this;
}

template<class T>
bool rkci::BigUFloat<T>::operator<(const BigUFloat<T> &other) const
{
	const int32_t thisTopDigit = m_lowPlace + m_numDigits;
	const int32_t otherTopDigit = other.m_lowPlace + other.m_numDigits;

	if (thisTopDigit < otherTopDigit)
		return true;
	if (thisTopDigit > otherTopDigit)
		return false;

	return CompareFirstMismatchedFragment(other, Comparer<Fragment_t>::Less);
}

template<class T>
bool rkci::BigUFloat<T>::operator<=(const BigUFloat<T> &other) const
{
	const int32_t thisTopDigit = m_lowPlace + m_numDigits;
	const int32_t otherTopDigit = other.m_lowPlace + other.m_numDigits;

	if (thisTopDigit < otherTopDigit)
		return true;
	if (thisTopDigit > otherTopDigit)
		return false;

	return !CompareFirstMismatchedFragment(other, Comparer<Fragment_t>::Greater);
}

template<class T>
bool rkci::BigUFloat<T>::operator>(const BigUFloat<T> &other) const
{
	return !((*this) <= other);
}

template<class T>
bool rkci::BigUFloat<T>::operator>=(const BigUFloat<T> &other) const
{
	return !((*this) < other);
}

template<class T>
bool rkci::BigUFloat<T>::operator==(const BigUFloat<T> &other) const
{
	if (this == &other)
		return true;

	if (m_lowPlace != other.m_lowPlace || m_numDigits != other.m_numDigits)
		return false;

	const size_t numFragments = m_fragments.Count();
	RKC_ASSERT(numFragments == other.m_fragments.Count());

	for (size_t i = 0; i < numFragments; i++)
		if (m_fragments[i] != other.m_fragments[i])
			return false;

	return true;
}

template<class T>
bool rkci::BigUFloat<T>::operator!=(const BigUFloat<T> &other) const
{
	return !((*this) == other);
}

// Cloner implementation
template<class T>
rkci::ResultRV<rkci::BigUFloat<T>> rkci::Cloner<rkci::BigUFloat<T>>::Clone(const BigUFloat<T> &t)
{
	RKC_CHECK_RV(BigUFloat<T>, clone, t.Clone());
	return clone;
}
