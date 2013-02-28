#pragma once
#include "../base/BaseHeader.h"
#include <typeinfo>

template <typename ByteWiseCopyArray>
bool SaveArrayToFile(ByteWiseCopyArray& pArray, FILE* pFile)
{
	UINT64 elementCnt = pArray.size();
	if (1 != fwrite(&elementCnt, sizeof(UINT64), 1, pFile))
		return false;

	if (pArray.empty())
		return true;

	if (pArray.size() == fwrite(&pArray[0], sizeof(pArray[0]), pArray.size(), pFile))
		return true;
	else
		return false;
}

template <typename ByteWiseCopyArray>
bool LoadArrayFromFile(ByteWiseCopyArray& pArray, FILE* pFile)
{
	UINT64 elementCnt = 0;
	if (1 != fread(&elementCnt, sizeof(UINT64), 1, pFile))
		return false;

	pArray.resize((size_t)elementCnt);
	if (0 == elementCnt)
		return true;
	if (elementCnt == fread(&pArray[0], sizeof(pArray[0]), (size_t)elementCnt, pFile))
		return true;
	else
		return false;
}

template <typename ByteWiseCopyType>
bool SaveTypeToFile(ByteWiseCopyType& data, FILE* pFile)
{
	if (1 == fwrite(&data, sizeof(ByteWiseCopyType), 1, pFile))
		return true;
	else
		return false;
}

template <typename ByteWiseCopyType>
bool LoadTypeFromFile(ByteWiseCopyType& data, FILE* pFile)
{
	if (1 == fread(&data, sizeof(ByteWiseCopyType), 1, pFile))
		return true;
	else
		return false;
}
