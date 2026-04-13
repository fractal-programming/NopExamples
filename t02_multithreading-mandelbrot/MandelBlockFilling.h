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

#ifndef MANDEL_BLOCK_FILLING_H
#define MANDEL_BLOCK_FILLING_H

#include <string>

#include "Processing.h"
#include "LibMandel.h"

const double cZoomFloatMax = 17000;

class MandelBlockFilling : public Processing
{

public:

	static MandelBlockFilling *create()
	{
		return new dNoThrow MandelBlockFilling;
	}

	// Input
	ConfigMandelbrot *mpCfg;

	uint32_t mIdxLine;
	char *mpLine;

	// Input
	uint32_t mNumIter;

	static void gradientBuild();

protected:

	virtual ~MandelBlockFilling() {}

private:

	MandelBlockFilling();
	MandelBlockFilling(const MandelBlockFilling &) = delete;
	MandelBlockFilling &operator=(const MandelBlockFilling &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	Success lineFill();
	void colorMandelbrotChunks(char *pData, uint32_t idxLine, uint32_t idxPixel, uint32_t numPixel);

	/* member variables */
	uint32_t mStartMs;
	uint32_t mNumElemPerBlock;
	uint32_t mNumBlock;
	uint32_t mIdxBlock;
	uint32_t mNumPixel;
	uint32_t mIdxPixel;
	char *mpData;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

