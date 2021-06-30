#include "CoreDefs.h"
#include "Result.h"
#include "DecBin.h"
#include "NumStr.h"
#include "ArraySliceView.h"
#include "BigUBinFloatProto.h"
#include "BigUDecFloatProto.h"
#include "BigUFloat.h"
#include "FloatSpec.h"
#include "MoveOrCopy.h"

#include <cstring>

namespace rkci
{
	namespace Tests
	{
		Result BigAtof(IAllocator &alloc)
		{
			NumStr numStr(alloc);

			rkci::BigUDecFloat_t f1(93456000, alloc);
			rkci::BigUDecFloat_t f2(93456001, alloc);
			rkci::FloatSpec singleSpec(8, 23, 127, true, true);

			const char *testNumber = "22223.511111111111111111111111111111";

			uint32_t numTrailingZeroes = 0;
			RKC_CHECK_RV(rkci::BigUDecFloat_t, resultNum, numStr.DecimalUTF8ToDecFloat(ArraySliceView<const uint8_t>(reinterpret_cast<const uint8_t*>(testNumber), strlen(testNumber)), numTrailingZeroes));
			RKC_CHECK_RV(rkci::BigUBinFloat_t, resultBin, DecBin::DecToBin(resultNum, singleSpec, numTrailingZeroes));

			RKC_CHECK_RV(rkci::BigUDecFloat_t, resultDec, DecBin::BinToDecWithFloatSpec(resultBin, singleSpec));

			return Result::Ok();

		}
	}
}
