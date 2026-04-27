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

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StFiltSendReadyWait) \
		gen(StMain) \
		gen(StNop) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

UserInteracting::UserInteracting()
	: Processing("UserInteracting")
	//, mStartMs(0)
	, mFd(INVALID_SOCKET)
	, mpFilt(NULL)
	, mpMbCreate(NULL)
	, mInSettings(false)
{
	mState = StStart;
}

/* member functions */

Success UserInteracting::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	PipeEntry<KeyUser> entKey;
	KeyUser key;
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

		msgMain();

		mState = StMain;

		break;
	case StMain:

		success = mpFilt->success();
		if (success != Pending)
			return Positive;

		if (mpFilt->ppKeys.get(entKey) < 1)
			break;
		key = entKey.particle;

		if (key == keyTab)
		{
			mInSettings = not mInSettings;
			msgMain();
			break;
		}

		break;
	case StNop:

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
	msg += "\r\n";

	if (mInSettings) msg += "\033[1m";
	msg += "  ---- Settings ---- \r\n";
	msg += "\033[0m";
	msg += "\r\n";

	msg += "> Setting 1      |             |\r\n";
	msg += "\r\n";

	msg += "  Filename: /tmp/<port>_1.bmp\r\n";
	msg += "\r\n";

	mpFilt->send(msg.c_str(), msg.size());
}

void UserInteracting::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

