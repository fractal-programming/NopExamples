/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 27.03.2026

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

#include "UserInteracting.h"
#include "LibTime.h"

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StFiltSendReadyWait) \
		gen(StMandelStart) \
		gen(StMandelDoneWaitStart) \
		gen(StMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

const double cStepNavigation = 0.2;
const uint32_t cDelayNavigationMs = 30;

UserInteracting::UserInteracting()
	: Processing("UserInteracting")
	//, mStartMs(0)
	, mFd(INVALID_SOCKET)
	, mpFilt(NULL)
	, mpMbCreate(NULL)
	, mNameFile("")
	, mInSettings(false)
{
	mState = StStart;
}

/* member functions */

Success UserInteracting::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	PipeEntry<KeyUser> entKey;
	KeyUser key;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (mFd == INVALID_SOCKET)
			return procErrLog(-1, "socket not set");

		mpFilt = TelnetFiltering::create(mFd);
		if (!mpFilt)
			return procErrLog(-1, "could not create process");

		mpFilt->procTreeDisplaySet(false);
		start(mpFilt);

		mState = StFiltSendReadyWait;

		break;
	case StFiltSendReadyWait:

		success = mpFilt->success();
		if (success != Pending)
			return success;

		if (!mpFilt->mSendReady)
			break;

		ok = nameFileGenerate();
		if (!ok)
			return procErrLog(-1, "could not generate filename");

		msgMain();

		mState = StMandelStart;

		break;
	case StMain:

		success = mpFilt->success();
		if (success != Pending)
			return Positive;

		if (mpFilt->ppKeys.get(entKey) < 1)
			break;
		key = entKey.particle;

		if (diffMs < cDelayNavigationMs)
			break;

		if (key == keyTab)
		{
			mInSettings = not mInSettings;
			msgMain();
			break;
		}

		if (key == 'd')
		{
			mCfg.zoom *= 1.2;
			mCfg.useDouble = mCfg.zoom > cZoomFloatMax || env.forceDouble;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'f')
		{
			mCfg.zoom /= 1.2;
			mCfg.useDouble = mCfg.zoom > cZoomFloatMax || env.forceDouble;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'g')
		{
			mCfg.posX = -0.74364388703715101;
			mCfg.posY = 0.0;
			mCfg.zoom = 0.5;
			mCfg.useDouble = false;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'h')
		{
			mCfg.posX -= cStepNavigation / mCfg.zoom;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'l')
		{
			mCfg.posX += cStepNavigation / mCfg.zoom;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'j')
		{
			mCfg.posY -= cStepNavigation / mCfg.zoom;
			msgMain();
			mState = StMandelStart;
			break;
		}

		if (key == 'k')
		{
			mCfg.posY += cStepNavigation / mCfg.zoom;
			msgMain();
			mState = StMandelStart;
			break;
		}

		break;
	case StMandelStart:

		ok = mandelbrotStart();
		if (!ok)
			return procErrLog(-1, "could not start mandelbrot creation");

		mState = StMandelDoneWaitStart;

		break;
	case StMandelDoneWaitStart:

		success = mpMbCreate->success();
		if (success == Pending)
			break;

		if (success != Positive)
			return procErrLog(-1, "could not create Mandelbrot picture");

		repel(mpMbCreate);

		mStartMs = curTimeMs;

		mState = StMain;

		break;
	default:
		break;
	}

	return Pending;
}

void UserInteracting::msgMain()
{
	string msg;

	msg += "\033[2J\033[H";

	msg += "\r\n";
	if (!mInSettings) msg += "\033[1m";
	msg += "  --- Navigation ---\r\n";
	msg += "\033[0m";
	msg += "\r\n";

	msg += "  [h,j,k,l] .. Left,Down,Up,Right\r\n";
	msg += "  [d,f]     .. Zoom: +,-\r\n";
	msg += "  [g]       .. Reset zoom\r\n";
	msg += "\r\n";

	if (mInSettings) msg += "\033[1m";
	msg += "  ---- Settings ---- \r\n";
	msg += "\033[0m";
	msg += "\r\n";

	char bufCfg[301];
	char *pBuf = bufCfg;
	char *pBufEnd = bufCfg + sizeof(bufCfg);

	*pBuf = 0;
	dInfo("  Pos X            %32.17f\r\n", mCfg.posX);
	dInfo("  Pos Y            %32.17f\r\n", mCfg.posY);
	dInfo("  Zoom             %14.3e\r\n", mCfg.zoom);
	dInfo("  Datatype         %14s%s\r\n",
					mCfg.useDouble ? "double" : "float",
					mCfg.forceDouble ? " (forced)" : "");
	msg += bufCfg;
	msg += "\r\n";

	msg += "  Filename: " + mNameFile + ".bmp\r\n";
	msg += "\r\n";

	mpFilt->send(msg.c_str(), msg.size());
}

bool UserInteracting::mandelbrotStart()
{
	mpMbCreate = MandelbrotCreating::create();
	if (!mpMbCreate)
	{
		procWrnLog("could not create process");
		return false;
	}

	mpMbCreate->mNameFile = mNameFile;

	mpMbCreate->mTypeDriver = env.typeDriver;
	mpMbCreate->mNumThreadsPool = env.numThreadsPool;
	mpMbCreate->mNumFillers = env.numFillers;
	mpMbCreate->mDisableGpu = env.disableGpu;

	mpMbCreate->mCfg = mCfg;

	start(mpMbCreate);

	return true;
}

bool UserInteracting::nameFileGenerate()
{
	struct sockaddr_storage addr;
	socklen_t addrLen;
	int res;
	bool ok;

	memset(&addr, 0, sizeof(addr));
	addrLen = sizeof(addr);

	res = ::getpeername(mFd, (struct sockaddr *)&addr, &addrLen);
#ifdef _WIN32
	if (res == SOCKET_ERROR)
#else
	if (res == -1)
#endif
	{
		procErrLog(-1, "could not get peer name");
		return false;
	}

	string addrRemote;
	uint16_t portRemote;
	bool isIPv6Remote;

	ok = TcpTransfering::sockaddrInfoGet(addr, addrRemote, portRemote, isIPv6Remote);
	if (!ok)
	{
		procErrLog(-1, "could not get address info");
		return false;
	}

	mNameFile = "/tmp/remote_";
#if 0
	mNameFile += to_string(portRemote);
#else
	mNameFile += "1";
#endif
	procDbgLog("Filename: %s", mNameFile.c_str());

	return true;
}

void UserInteracting::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

