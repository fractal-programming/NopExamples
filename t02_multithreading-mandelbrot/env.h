/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 31.03.2018

  Copyright (C) 2018, Johannes Natter
*/

#ifndef ENV_H
#define ENV_H

#include <string>

/*
 * ##################################
 * Environment contains variables for
 *          !! IPC ONLY !!
 * ##################################
 */

struct Environment
{
	// Default
	bool haveTclap;
	bool daemonDebug;
	int verbosity;
#if defined(__unix__)
	bool coreDump;
#endif

	// Application
	std::string nameFile;
	std::string dirOutput;
	bool forceDouble;
#if APP_HAS_AVX2
	bool disableSimd;
#endif
#if APP_HAS_VULKAN
	std::string nameFileShader;
	bool disableGpu;
	bool disableCacheShader;
#endif
	uint16_t port;

	uint32_t imgWidth;
	uint32_t imgHeight;
	double posX;
	double posY;
	double zoom;

	std::string typeDriver;
	uint32_t numIterMax;
	uint32_t numThreadsPool;
	uint32_t numFillers;
	uint32_t numBurst;
};

extern Environment env;

#endif

