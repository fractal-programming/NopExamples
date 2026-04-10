/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 31.03.2026

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

#ifndef APP_HAS_TCLAP
#define APP_HAS_TCLAP 0
#endif

#if defined(__unix__)
#include <signal.h>
#endif
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif
#include <iostream>
#include <chrono>
#include <thread>
#if APP_HAS_TCLAP
#include <tclap/CmdLine.h>
#endif

#if APP_HAS_TCLAP
#include "TclapOutput.h"
#endif
#include "Supervising.h"
#include "LibDspc.h"

#include "env.h"

using namespace std;
using namespace chrono;
#if APP_HAS_TCLAP
using namespace TCLAP;
#endif

#define dFileExtDefault "cpp"

Environment env;
Supervising *pApp = NULL;

#if APP_HAS_TCLAP
class AppHelpOutput : public TclapOutput {};
#endif

#if defined(_WIN32)
BOOL WINAPI applicationCloseRequest(DWORD signal)
{
	if (signal != CTRL_C_EVENT)
		return FALSE;

	cout << endl;
	pApp->unusedSet();

	return TRUE;
}
#else
void applicationCloseRequest(int signum)
{
	(void)signum;

	cout << endl;
	pApp->unusedSet();
}
#endif

int main(int argc, char *argv[])
{
	env.haveTclap = 1;
	env.verbosity = 0;
#if defined(__unix__)
	env.coreDump = false;
#endif
	env.fileExt = dFileExtDefault;

#if APP_HAS_TCLAP
	CmdLine cmd("Listing of project metrics", ' ', appVersion());

	AppHelpOutput aho;
	aho.package = dPackageName;
	aho.versionApp = dVersion;
	aho.nameApp = dAppName;
	aho.copyright = " (C) 2026 DSP-Crowd Electronics GmbH";
	cmd.setOutput(&aho);

	ValueArg<int> argVerbosity("v", "verbosity", "Verbosity: high => more output", false, 0, "uint");
	cmd.add(argVerbosity);
#if defined(__unix__)
	SwitchArg argCoreDump("", "core-dump", "Enable core dumps", false);
	cmd.add(argCoreDump);
#endif

	UnlabeledValueArg<string> argExt("ext",
		"File extension to search for. Default: " dFileExtDefault,
		false, dFileExtDefault, "string");
	cmd.add(argExt);

	cmd.parse(argc, argv);

	int res = argVerbosity.getValue();
	if (res >= 0 && res < 6)
		env.verbosity = res;
#if defined(__unix__)
	env.coreDump = argCoreDump.getValue();
#endif
	env.fileExt = argExt.getValue();
#else
	env.haveTclap = 0;
	env.verbosity = 0;

	if (argc >= 2)
		env.fileExt = string(argv[1]);
#endif
	levelLogSet(env.verbosity);

#if defined(_WIN32)
	BOOL okWin = SetConsoleCtrlHandler(applicationCloseRequest, TRUE);
	if (!okWin) {
		errLog(-1, "could not set ctrl handler");
		return 1;
	}
#else
	signal(SIGINT, applicationCloseRequest);
	signal(SIGTERM, applicationCloseRequest);
#endif

	pApp = Supervising::create();
	if (!pApp) {
		errLog(-1, "could not create process");
		return 1;
	}

	while (1) {
		for (int i = 0; i < 13; ++i)
			pApp->treeTick();

		if (!pApp->progress())
			break;

		this_thread::sleep_for(milliseconds(15));
	}

	Success success = pApp->success();
	Processing::destroy(pApp);

	Processing::applicationClose();

	filesStdClose();

	return !(success == Positive);
}

