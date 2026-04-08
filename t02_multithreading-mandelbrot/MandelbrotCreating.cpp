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

MandelbrotCreating::MandelbrotCreating()
	: Processing("MandelbrotCreating")
	, mNameFile()
	, mTypeDriver("")
	, mNumThreadsPool(0)
	, mNumFillers(0)
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
	cfg.imgWidth = 1920;
	cfg.imgHeight = 1200;

	// Mandelbrot
	cfg.numIterMax = 2000;
	cfg.posX = -0.743643887037151;
	cfg.posY = 0.131825904205330;
	cfg.zoom = 170000;

	// Filling
	cfg.numBurst = 300;

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

		cfg.w2 = ((MbValFull)cfg.imgWidth) / 2;
		cfg.h2 = ((MbValFull)cfg.imgHeight) / 2;
		cfg.scaleX = 1.0 / cfg.zoom;
		cfg.scaleY = (cfg.scaleX * cfg.imgHeight) / (cfg.imgWidth * cfg.h2);
		cfg.scaleX /= cfg.w2;

		cfg.szData = cfg.imgWidth * cNumBytesPerPixel;
		cfg.szLine = ((cfg.szData + maskLine) & ~maskLine);
		cfg.szPadding = cfg.szLine - cfg.szData;

		mSzBuffer = cfg.szLine * mNumFillers;

		mpBuffer = new dNoThrow char[mSzBuffer];
		if (!mpBuffer)
			return procErrLog(-1, "could not allocate data buffer");

		mpBufferEnd = mpBuffer + mSzBuffer;
#if 0
		procDbgLog("Data size        %u", cfg.szData);
		procDbgLog("Line padding     %u", cfg.szPadding);

		procDbgLog("Line size        %u", cfg.szLine);
		procDbgLog("Buffer size      %u", mSzBuffer);

		procDbgLog("Buffer start     %p", mpBuffer);
		procDbgLog("Buffer end       %p", mpBufferEnd);
#endif
		mBmp.width = cfg.imgWidth;
		mBmp.height = cfg.imgHeight;

		mNameFile += ".bmp";
		ok = FileBmp::create(mNameFile.c_str(), &mBmp);
		if (!ok)
			return procErrLog(-1, "could not create BMP file");

		mStartMs = curTimeMs;

#if APP_HAS_VULKAN
		if (!cfg.disableGpu)
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
	InstanceVulkan inst;
	bool ok;

	gpuAvEnabledSet();

	inst = instanceVulkanGet();
	if (!inst.ok)
		return procErrLog(-1, "could not create Vulkan instance");

	devicesVulkanList(inst);

	DeviceVulkan dev;
	string shader;

	(void)DeviceVulkan::selectAndRegister(inst, "main", NULL,
						VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

	dev = DeviceVulkan::get("main");
	procDbgLog("Selected device: %s", dev.name().c_str());

	mpCompute = VulkanComputing::create(dev);
	if (!mpCompute)
		return procErrLog(-1, "could not create process");

	ok = mpCompute->bufferInAdd(0, &cfg, sizeof(cfg));
	if (!ok)
		return procErrLog(-1, "could not add configuration buffer");

	GradientStop *pStartGrad;
	size_t numElemGrad;

	gradientsGet(pStartGrad, numElemGrad);

	ok = mpCompute->bufferInAdd(1, pStartGrad, numElemGrad * sizeof(GradientStop));
	if (!ok)
		return procErrLog(-1, "could not add gradient buffer");

	ok = mpCompute->fileShaderAdd("../mandelbrot.comp");
	if (!ok)
		return procErrLog(-1, "could not add Mandelbrot compute shader");

	mpCompute->infoDebugShaders = true;

	start(mpCompute);

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

		ok = mBmp.lineAppend(pFill->mpLine, cfg.szLine);
		if (!ok)
			return procErrLog(-1, "could not append line");

		repel(pFill);
		iter = mLstFillers.erase(iter);

		++mIdxLineDone;
	}

	// Start new filler

	while (mLstFillers.size() < mNumFillers && mIdxLineFiller < cfg.imgHeight)
	{
#if 1
		if (useExt && ThreadPooling::queueReqFull())
			break;
#endif
		pFill = MandelBlockFilling::create();
		if (!pFill)
			return procErrLog(-1, "could not create process");

		pFill->mpCfg = &cfg;

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

		mpLineFiller += cfg.szLine;
		if (mpLineFiller >= mpBufferEnd)
			mpLineFiller = mpBuffer;
	}

	// Check overall finished

	if (mIdxLineDone < cfg.imgHeight)
		return Pending;

	return Positive;
}

Success MandelbrotCreating::argumentsCheck()
{
	if (!cfg.imgWidth || !cfg.imgHeight)
		return procErrLog(-1, "bad image format");

	if (cfg.numIterMax > 200000)
		return procErrLog(-1, "max. iterations too high");

	if (!mNumFillers || mNumFillers > cfg.imgHeight)
	{
		procDbgLog("setting number of fillers to number of lines");
		mNumFillers = cfg.imgHeight;
	}

	return Positive;
}

void MandelbrotCreating::processInfo(char *pBuf, char *pBufEnd)
{
#if 0
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	pBuf += progressStr(pBuf, pBufEnd, (int)mIdxLineFiller, (int)mBmp.height);
	dInfo("\n");

	pBuf += progressStr(pBuf, pBufEnd, (int)mIdxLineDone, (int)mBmp.height);
	dInfo("\n");
}

/* static functions */

