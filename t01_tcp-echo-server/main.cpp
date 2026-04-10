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

#define dPortListeningDefault "5000"
const int cPortMax = 64000;

Environment env;
Supervising *pApp = NULL;

#if APP_HAS_TCLAP
class AppHelpOutput : public TclapOutput {};
#endif

// OS signal handler => Tell the application what to do on Ctrl-C
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

void licensesPrint()
{
	cout << endl;
	cout << "This program uses the following external components" << endl;
	cout << endl;

	cout << "TCLAP" << endl;
	cout << "https://tclap.sourceforge.net/" << endl;
	cout << "MIT" << endl;
	cout << endl;
}

int main(int argc, char *argv[])
{
	// Register OS signal handlers
#if defined(_WIN32)
	// https://learn.microsoft.com/en-us/windows/console/setconsolectrlhandler
	BOOL okWin = SetConsoleCtrlHandler(applicationCloseRequest, TRUE);
	if (!okWin)
	{
		errLog(-1, "could not set ctrl handler");
		return 1;
	}
#else
	// http://man7.org/linux/man-pages/man7/signal.7.html
	signal(SIGINT, applicationCloseRequest);
	signal(SIGTERM, applicationCloseRequest);
#endif
	env.haveTclap = 1;
	env.daemonDebug = false;
	env.verbosity = 3;
#if defined(__unix__)
	env.coreDump = false;
#endif
	env.portListening = atoi(dPortListeningDefault);

#if APP_HAS_TCLAP
	int res;

	CmdLine cmd("Command description message", ' ', appVersion());

	AppHelpOutput aho;
#if 1
	aho.package = dPackageName;
	aho.versionApp = dVersion;
	aho.nameApp = dAppName;
	aho.copyright = " (C) 2025 DSP-Crowd Electronics GmbH";
#endif
	cmd.setOutput(&aho);

	SwitchArg argDebug("d", "debug", "Enable debugging daemon", false);
	cmd.add(argDebug);
	ValueArg<int> argVerbosity("v", "verbosity", "Verbosity: high => more output", false, 3, "int");
	cmd.add(argVerbosity);
	SwitchArg argLicenses("", "licenses", "Show dependencies", false);
	cmd.add(argLicenses);
#if defined(__unix__)
	SwitchArg argCoreDump("", "core-dump", "Enable core dumps", false);
	cmd.add(argCoreDump);
#endif
	ValueArg<int> argPortListening("", "port-listening", "Listening port of TCP server. Default: " dPortListeningDefault,
								false, env.portListening, "int");
	cmd.add(argPortListening);

	cmd.parse(argc, argv);

	if (argLicenses.getValue())
	{
		licensesPrint();
		return 0;
	}

	env.daemonDebug = argDebug.getValue();

	res = argVerbosity.getValue();
	if (res >= 0 && res < 6)
		env.verbosity = res;
#if defined(__unix__)
	env.coreDump = argCoreDump.getValue();
#endif
	res = argPortListening.getValue();
	if (res > 0 && res <= cPortMax)
		env.portListening = res;
#else
	(void)argc;
	(void)argv;

	env.haveTclap = 0;
#endif
	levelLogSet(env.verbosity);

	pApp = Supervising::create();
	if (!pApp)
	{
		cerr << "could not create process" << endl;
		return 1;
	}

	while (1)
	{
		for (size_t coreBurst = 0; coreBurst < 13; ++coreBurst)
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

