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

Environment env;
Supervising *pApp = NULL;

#if APP_HAS_TCLAP
class AppHelpOutput : public TclapOutput {};
#endif

#define cNameFileDefault			"mandelbrot_1"
#define cFileShaderDefault		"../mandelbrot.comp"
#define cDirOutputDefault		"."
#define cImgWidthDefault			"1920"
#define cImgHeightDefault		"1200"
#define cPosXDefault			"-0.743643887037151"
#define cPosYDefault			"0.131825904205330"
#define cZoomDefault			"170000"
#define cTypeDriverDefault		"ext"
#define cNumIterMaxDefault		"2000"
#define cNumThreadsPoolDefault	"20"
#define cNumFillersDefault		cImgHeightDefault
#define cNumBurstDefault		"300"

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
	env.daemonDebug = false;
	env.verbosity = 3;
#if defined(__unix__)
	env.coreDump = false;
#endif

#if APP_HAS_TCLAP
	env.haveTclap = 1;

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

	// Default
	SwitchArg argDebug("d", "debug", "Enable debugging daemon", false);
	cmd.add(argDebug);
	ValueArg<int> argVerbosity("v", "verbosity", "Verbosity: high => more output", false, 3, "uint");
	cmd.add(argVerbosity);
	SwitchArg argLicenses("", "licenses", "Show dependencies", false);
	cmd.add(argLicenses);
#if defined(__unix__)
	SwitchArg argCoreDump("", "core-dump", "Enable core dumps", false);
	cmd.add(argCoreDump);
#endif

	// Application
	ValueArg<string> argFileOut("", "file-out", "Name of the output file without extension. Default: " cNameFileDefault,
								false, cNameFileDefault, "string");
	cmd.add(argFileOut);
	ValueArg<string> argDirOut("", "dir-out", "Output directory. Default: " cDirOutputDefault,
								false, cDirOutputDefault, "string");
	cmd.add(argDirOut);
	SwitchArg argForceDouble("", "double", "Force calculation in double", false);
	cmd.add(argForceDouble);
#if APP_HAS_AVX2
	SwitchArg argDisableSimd("", "no-simd", "Disable usage of SIMD (Single Instruction Multiple Data)", false);
	cmd.add(argDisableSimd);
#endif
#if APP_HAS_VULKAN
	ValueArg<string> argFileShader("", "file-shader", "Name of the shader file. Default: " cFileShaderDefault,
								false, cFileShaderDefault, "string");
	cmd.add(argFileShader);
	SwitchArg argDisableGpu("", "no-gpu", "Disable usage of GPU", false);
	cmd.add(argDisableGpu);
	SwitchArg argDisableCacheShader("", "no-shader-cache", "Disable caching of shader files", false);
	cmd.add(argDisableCacheShader);
#endif
	ValueArg<uint16_t> argPort("", "port-telnet", "Start in server mode if not zero. Default: 0", false, 0, "uint");
	cmd.add(argPort);
	ValueArg<size_t> argImgWidth("", "img-width", "Width of generated image. Default: " cImgWidthDefault,
								false, atoi(cImgWidthDefault), "uint");
	cmd.add(argImgWidth);
	ValueArg<size_t> argImgHeight("", "img-height", "Height of generated image. Default: " cImgHeightDefault,
								false, atoi(cImgHeightDefault), "uint");
	cmd.add(argImgHeight);
	ValueArg<double> argPosX("", "pos-x", "X-Position in the complex plane. Default: " cPosXDefault,
								false, atof(cPosXDefault), "double");
	cmd.add(argPosX);
	ValueArg<double> argPosY("", "pos-y", "Y-Position in the complex plane. Default: " cPosYDefault,
								false, atof(cPosYDefault), "double");
	cmd.add(argPosY);
	ValueArg<double> argZoom("", "zoom", "Zoom in the complex plane. Default: " cZoomDefault,
								false, atof(cZoomDefault), "double");
	cmd.add(argZoom);
	ValueArg<string> argTypeDriver("", "type-driver",
								"Type of driver for each filler process. Default: " cTypeDriverDefault
								"\npar = Parent       (main thread)"
								"\nnew = NewInternal  (one thread per filler)"
								"\next = External     (thread pool)",
								false, cTypeDriverDefault, "string");
	cmd.add(argTypeDriver);
	ValueArg<size_t> argNumIterMax("", "iter-max", "Maximum number of Mandelbrot iterations per pixel. Default: " cNumIterMaxDefault,
								false, atoi(cNumIterMaxDefault), "uint");
	cmd.add(argNumIterMax);
	ValueArg<size_t> argThreadsPool("", "num-threads-pool", "Number of threads used by the thread-pool. Default: " cNumThreadsPoolDefault,
								false, atoi(cNumThreadsPoolDefault), "uint");
	cmd.add(argThreadsPool);
	ValueArg<size_t> argNumFillers("", "num-fillers", "Number of parallel line filler processes. Default: " cNumFillersDefault,
								false, atoi(cNumFillersDefault), "uint");
	cmd.add(argNumFillers);
	ValueArg<size_t> argNumBurst("", "num-burst", "Pixel blocks processed per filler per scheduler tick. Default: " cNumBurstDefault,
								false, atoi(cNumBurstDefault), "uint");
	cmd.add(argNumBurst);

	// Parse
	cmd.parse(argc, argv);

	// Default
	if (argLicenses.getValue())
	{
		licensesPrint();
		return 0;
	}

	env.daemonDebug = argDebug.getValue();

	res = argVerbosity.getValue();
	env.verbosity = PMIN(PMAX(res, 1), 5);
#if defined(__unix__)
	env.coreDump = argCoreDump.getValue();
#endif

	// Application
	env.nameFile = argFileOut.getValue();
	env.dirOutput = argDirOut.getValue();
	env.forceDouble = argForceDouble.getValue();
#if APP_HAS_AVX2
	env.disableSimd = argDisableSimd.getValue();
#endif
#if APP_HAS_VULKAN
	env.nameFileShader = argFileShader.getValue();
	env.disableGpu = argDisableGpu.getValue();
	env.disableCacheShader = argDisableCacheShader.getValue();
#endif
	env.port = argPort.getValue();

	env.imgWidth = argImgWidth.getValue();
	env.imgHeight = argImgHeight.getValue();
	env.posX = argPosX.getValue();
	env.posY = argPosY.getValue();
	env.zoom = argZoom.getValue();

	env.typeDriver = argTypeDriver.getValue();
	env.numIterMax = argNumIterMax.getValue();
	env.numThreadsPool = argThreadsPool.getValue();
	env.numFillers = argNumFillers.getValue();
	env.numBurst = argNumBurst.getValue();
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

		this_thread::sleep_for(milliseconds(2));
	}

	Success success = pApp->success();
	Processing::destroy(pApp);

	Processing::applicationClose();

	filesStdClose();

	return !(success == Positive);
}

