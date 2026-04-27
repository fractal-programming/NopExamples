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

#include <fstream>
#include <sstream>

#include "MandelbrotCreating.h"
#include "ThreadPooling.h"
#include "LibTime.h"
#include "LibDspc.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StCpuFillers) \
		gen(StVulkanStart) \
		gen(StVulkanDoneWait) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

#define dIdBufferConfig		0
#define dIdBufferGradients	1
#define dIdBufferDebug		5
#define dIdBufferResults		6
#define dIdBufferStats		7

MandelbrotCreating::MandelbrotCreating()
	: Processing("MandelbrotCreating")
	, mNameFile()
	, mTypeDriver("")
	, mNumThreadsPool(0)
	, mNumFillers(0)
	, mDisableGpu(false)
	, mIdxLineDone(0)
	, mNumIterations(0)
	, mDurationMs(0)
	, mStartMs(0)
	, mpBuffer(NULL)
	, mpBufferEnd(NULL)
	, mSzBuffer(0)
	, mBmp()
	, mLstFillers()
	, mIdxLineFiller(0)
	, mpLineFiller(NULL)
#if APP_HAS_VULKAN
	, mpCompute(NULL)
#endif
{
	// Image
	mCfg.imgWidth = 1920;
	mCfg.imgHeight = 1200;

	// Mandelbrot
	mCfg.numIterMax = 2000;
	mCfg.posX = -0.743643887037151;
	mCfg.posY = 0.131825904205330;
	mCfg.zoom = 170000;

	// Filling
	mCfg.numBurst = 300;

	mState = StStart;
}

MandelbrotCreating::~MandelbrotCreating()
{
	if (!mpBuffer)
		return;

	delete[] mpBuffer;
}

/* member functions */

Success MandelbrotCreating::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	bool ok;
	size_t maskLine = 3;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		success = argumentsCheck();
		if (success != Positive)
			return procErrLog(-1, "invalid arguments");

		mCfg.w2 = ((MbValFull)mCfg.imgWidth) / 2;
		mCfg.h2 = ((MbValFull)mCfg.imgHeight) / 2;
		mCfg.scaleX = 1.0 / mCfg.zoom;
		mCfg.scaleY = (mCfg.scaleX * mCfg.imgHeight) / (mCfg.imgWidth * mCfg.h2);
		mCfg.scaleX /= mCfg.w2;

		mCfg.szData = mCfg.imgWidth * cNumBytesPerPixel;
		mCfg.szLine = ((mCfg.szData + maskLine) & ~maskLine);
		mCfg.szPadding = mCfg.szLine - mCfg.szData;

		mSzBuffer = mCfg.szLine * mNumFillers;

		mpBuffer = new dNoThrow char[mSzBuffer];
		if (!mpBuffer)
			return procErrLog(-1, "could not allocate data buffer");

		mpBufferEnd = mpBuffer + mSzBuffer;
#if 0
		procDbgLog("Data size        %u", mCfg.szData);
		procDbgLog("Line padding     %u", mCfg.szPadding);

		procDbgLog("Line size        %u", mCfg.szLine);
		procDbgLog("Buffer size      %u", mSzBuffer);

		procDbgLog("Buffer start     %p", mpBuffer);
		procDbgLog("Buffer end       %p", mpBufferEnd);
#endif
		mNameFile += ".bmp";
		ok = mBmp.writeOpen(mNameFile.c_str(), mCfg.imgWidth, mCfg.imgHeight);
		if (!ok)
			return procErrLog(-1, "could not create BMP file");

		mStartMs = curTimeMs;

#if APP_HAS_VULKAN
		if (!mDisableGpu)
		{
			mState = StVulkanStart;
			break;
		}
#endif
		mIdxLineFiller = 0;
		mpLineFiller = mpBuffer;

		mState = StCpuFillers;

		break;
	case StCpuFillers:

		success = fillersProcess();
		if (success == Pending)
			break;

		if (success != Positive)
			return procErrLog(-1, "could not process lines");

		mDurationMs = diffMs;

		return Positive;

#if APP_HAS_VULKAN
		break;
	case StVulkanStart:

		success = vulkanStart();
		if (success != Positive)
			return procErrLog(-1, "could not start Vulkan computing");

		mState = StVulkanDoneWait;

		break;
	case StVulkanDoneWait:

		success = mpCompute->success();
		if (success == Pending)
			break;

		if (success != Positive)
			return procErrLog(-1, "could not compute Mandelbrot set");

		// Consume result
		success = resultVulkanWrite();
		if (success != Positive)
			return procErrLog(-1, "could not write Mandelbrot image");

		// Repel
		repel(mpCompute);

		mIdxLineDone = mCfg.imgHeight;

		mDurationMs = diffMs;

		return Positive;
#endif
		break;
	default:
		break;
	}

	return Pending;
}

#if APP_HAS_VULKAN
Success MandelbrotCreating::vulkanStart()
{
	GradientStop *pStartGrad;
	size_t numElemGrad;

	gradientsGet(pStartGrad, numElemGrad);

	mpCompute = VulkanComputing::create("main");
	if (!mpCompute)
		return procErrLog(-1, "could not create process");

	uint32_t numWorkgroupsX = (mCfg.imgWidth  + 15) / 16;
	uint32_t numWorkgroupsY = (mCfg.imgHeight + 15) / 16;

	mpCompute->workgroupsSet(numWorkgroupsX, numWorkgroupsY);

	mpCompute->shaderUse("mandel");

	// Input
	mpCompute->bufferInAdd(dIdBufferConfig, sizeof(mCfg), &mCfg);
	mpCompute->bufferInAdd(dIdBufferGradients, numElemGrad * sizeof(GradientStop), pStartGrad);

	//hexDump(&mCfg, sizeof(mCfg), "Config");

	// Output
	mpCompute->bufferOutAdd(dIdBufferDebug, 128);
	mpCompute->bufferOutAdd(dIdBufferResults, mCfg.szLine * mCfg.imgHeight);
	mpCompute->bufferOutAdd(dIdBufferStats, sizeof(uint32_t) * mCfg.imgHeight);

	start(mpCompute);

	return Positive;
}

Success MandelbrotCreating::resultVulkanWrite()
{
	const char *pResult;
	uint32_t *pNumIter;
	size_t sz;
#if 0
	pResult = mpCompute->bufferMap(dIdBufferDebug, &sz);
	if (!pResult)
		return procErrLog(-1, "could not map result");

	hexDump(pResult, sz, "Debug");
	mpCompute->bufferUnmap(dIdBufferDebug);
#endif
	pResult = (const char *)mpCompute->bufferMap(dIdBufferResults, &sz);
	if (!pResult)
		return procErrLog(-1, "could not map result");

	pNumIter = (uint32_t *)mpCompute->bufferMap(dIdBufferStats, &sz);
	if (!pNumIter)
	{
		mpCompute->bufferUnmap(dIdBufferResults);
		return procErrLog(-1, "could not map statistics");
	}

	//hexDump(pNumIter, sz, "Statistics");

	size_t i;
	bool ok;

	mNumIterations = 0;

	i = 0;
	for (; i < mCfg.imgHeight; ++i, pResult += mCfg.szLine, ++pNumIter)
	{
		ok = mBmp.lineWrite(pResult, mCfg.szLine);
		if (!ok)
			return procErrLog(-1, "could not write line");

		mNumIterations += *pNumIter;
	}

	mpCompute->bufferUnmap(dIdBufferResults);
	mpCompute->bufferUnmap(dIdBufferStats);

	return Positive;
}
#endif

Success MandelbrotCreating::shutdown()
{
	mBmp.close();
	return Positive;
}

Success MandelbrotCreating::fillersProcess()
{
	list<MandelBlockFilling *>::iterator iter;
	MandelBlockFilling *pFill;
	Success success;
	bool ok, useExt;

	useExt = mTypeDriver == "ext" && mNumThreadsPool;

	// Check filler finished

	iter = mLstFillers.begin();

	while (1)
	{
		if (iter == mLstFillers.end())
			break;

		pFill = *iter;

		success = pFill->success();
		if (success == Pending)
			break;

		if (success != Positive)
			return procErrLog(-1, "error filling line %u @ %p",
								pFill->mIdxLine, pFill->mpLine);

		mNumIterations += pFill->mNumIter;

		ok = mBmp.lineWrite(pFill->mpLine, mCfg.szLine);
		if (!ok)
			return procErrLog(-1, "could not write line");

		repel(pFill);
		iter = mLstFillers.erase(iter);

		++mIdxLineDone;
	}

	// Start new filler

	while (mLstFillers.size() < mNumFillers && mIdxLineFiller < mCfg.imgHeight)
	{
#if 1
		if (useExt && ThreadPooling::queueReqFull())
			break;
#endif
		pFill = MandelBlockFilling::create();
		if (!pFill)
			return procErrLog(-1, "could not create process");

		pFill->mpCfg = &mCfg;

		pFill->mIdxLine = mIdxLineFiller;
		pFill->mpLine = mpLineFiller;

		pFill->procTreeDisplaySet(false);

		if (useExt)
		{
			start(pFill, DrivenByExternalDriver);

			ssize_t numDone = ThreadPooling::procAdd(pFill);
			if (numDone != 1)
			{
				procDbgLog("external driver request failed. switching back to parental drive");
				start(pFill);
			}
		}
		else
		if (mTypeDriver == "new")
			start(pFill, DrivenByNewInternalDriver);
		else
			start(pFill);

		mLstFillers.push_back(pFill);

		// Next line
		++mIdxLineFiller;

		mpLineFiller += mCfg.szLine;
		if (mpLineFiller >= mpBufferEnd)
			mpLineFiller = mpBuffer;
	}

	// Check overall finished

	if (mIdxLineDone < mCfg.imgHeight)
		return Pending;

	return Positive;
}

Success MandelbrotCreating::argumentsCheck()
{
	if (!mCfg.imgWidth || !mCfg.imgHeight)
		return procErrLog(-1, "bad image format");

	if (mCfg.numIterMax > 200000)
		return procErrLog(-1, "max. iterations too high");

	if (!mNumFillers || mNumFillers > mCfg.imgHeight)
	{
		procDbgLog("setting number of fillers to number of lines");
		mNumFillers = mCfg.imgHeight;
	}

	return Positive;
}

void MandelbrotCreating::processInfo(char *pBuf, char *pBufEnd)
{
#if 0
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	pBuf += progressStr(pBuf, pBufEnd, (int)mIdxLineFiller, (int)mCfg.imgHeight);
	dInfo("\n");

	pBuf += progressStr(pBuf, pBufEnd, (int)mIdxLineDone, (int)mCfg.imgHeight);
	dInfo("\n");

	(void)pBuf;
}

/* static functions */

