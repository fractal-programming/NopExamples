/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 14.03.2026

  Copyright (C) 2026, Johannes Natter

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MandelBlockFilling.h"
#include "LibBmp.h"
#include "LibDspc.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StIdle) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

MandelBlockFilling::MandelBlockFilling()
	: Processing("MandelBlockFilling")
	, mpCfg(NULL)
	, mIdxLine(0)
	, mpLine(NULL)
	, mNumIter(0)
	, mStartMs(0)
	, mNumElemPerBlock(0)
	, mNumBlock(0)
	, mIdxBlock(0)
	, mNumPixel(0)
	, mIdxPixel(0)
	, mpData(NULL)
{
	mState = StStart;
}

/* member functions */

Success MandelBlockFilling::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (!mpCfg || !mpLine)
			return procErrLog(-1, "invalid arguments");

		mpData = mpLine;

		mNumElemPerBlock = cNumDoublesPerBlock;
		if (!mpCfg->useDouble)
			mNumElemPerBlock *= 2;

		mNumPixel = mpCfg->szData / cNumBytesPerPixel;
		mIdxPixel = 0;

		mNumBlock = ((mNumPixel + cMaskElem) >> cShiftElem);
		mIdxBlock = 0;

		mStartMs = curTimeMs;
		mState = StMain;

		break;
	case StMain:

		if (!mIdxLine && diffMs > 25) procDbgLog("sleeped");
		mStartMs = curTimeMs;

		success = lineFill();
		if (success == Pending)
			break;

		return Positive;

		break;
	case StIdle:

		break;
	default:
		break;
	}

	return Pending;
}

Success MandelBlockFilling::lineFill()
{
	uint32_t numRemaining, numBurst; // Unit: Blocks
	uint32_t numPixelProcessed;

	numRemaining = mNumBlock - mIdxBlock;
	numBurst = PMIN(numRemaining, mpCfg->numBurst);

	const char *pDataEnd = mpLine + mpCfg->szLine;

	while (numRemaining = mNumPixel - mIdxPixel, numRemaining && numBurst)
	{
		numPixelProcessed = PMIN(numRemaining, mNumElemPerBlock);

		colorMandelbrotChunks(mpData, mIdxLine, mIdxPixel, numPixelProcessed);

		mIdxPixel += numPixelProcessed;
		mpData += cNumBytesPerPixel * numPixelProcessed;

		++mIdxBlock;
		--numBurst;
	}

	if (mIdxPixel < mNumPixel)
		return Pending;

	for (; mpData < pDataEnd; ++mpData)
		*mpData = 0;

	return Positive;
}

void MandelBlockFilling::colorMandelbrotChunks(char *pData, uint32_t idxLine, uint32_t idxPixel, uint32_t numPixel)
{
#if APP_HAS_AVX2
	if (numPixel == mNumElemPerBlock && !mpCfg->disableSimd)
	{
		mNumIter += colorMandelbrotSimd(mpCfg, pData, idxLine, idxPixel);
		return;
	}
#endif
	for (uint32_t i = 0; i < numPixel; ++i)
	{
		mNumIter += colorMandelbrotScalar(mpCfg, pData, idxLine, idxPixel);

		pData += cNumBytesPerPixel;
		++idxPixel;
	}
}

void MandelBlockFilling::processInfo(char *pBuf, char *pBufEnd)
{
#if 0
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	dInfo("%03u: ", mIdxLine);
	pBuf += progressStr(pBuf, pBufEnd, (int)mIdxPixel, (int)mNumPixel);
	dInfo("\n");
	(void)pBuf;
}

/* static functions */

