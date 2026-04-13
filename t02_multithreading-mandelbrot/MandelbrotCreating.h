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

#ifndef MANDELBROT_CREATING_H
#define MANDELBROT_CREATING_H

#include <string>
#include <list>

#include "Processing.h"
#include "MandelBlockFilling.h"
#include "LibBmp.h"
#if APP_HAS_VULKAN
#include "VulkanComputing.h"
#endif

class MandelbrotCreating : public Processing
{

public:

	static MandelbrotCreating *create()
	{
		return new dNoThrow MandelbrotCreating;
	}

	// Input
	std::string mNameFile;
	ConfigMandelbrot cfg;

	std::string mTypeDriver;
	uint32_t mNumThreadsPool;
	uint32_t mNumFillers;

	// Output

	// Monitoring
	uint32_t mIdxLineDone;

	// Result
	size_t mNumIterations;
	uint32_t mDurationMs;

protected:

	virtual ~MandelbrotCreating();

private:

	MandelbrotCreating();
	MandelbrotCreating(const MandelbrotCreating &) = delete;
	MandelbrotCreating &operator=(const MandelbrotCreating &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	Success fillersProcess();
	Success argumentsCheck();
#if APP_HAS_VULKAN
	Success vulkanStart();
#endif
	/* member variables */
	uint32_t mStartMs;
	char *mpBuffer;
	char *mpBufferEnd;
	uint32_t mSzBuffer;
	FileBmp mBmp;

	std::list<MandelBlockFilling *> mLstFillers;

	uint32_t mIdxLineFiller;
	char *mpLineFiller;
#if APP_HAS_VULKAN
	VulkanComputing *mpCompute;
#endif
	/* static functions */

	/* static variables */

	/* constants */

};

#endif

