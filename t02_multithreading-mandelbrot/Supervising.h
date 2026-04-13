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

#ifndef SUPERVISING_H
#define SUPERVISING_H

#include "Processing.h"
#include "MandelbrotCreating.h"
#include "TcpListening.h"
#if APP_HAS_VULKAN
#include "ShaderCompiling.h"
#endif

class Supervising : public Processing
{

public:

	static Supervising *create()
	{
		return new dNoThrow Supervising;
	}

protected:

	virtual ~Supervising() {}

private:

	Supervising();
	Supervising(const Supervising &) = delete;
	Supervising &operator=(const Supervising &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	bool basicsStart();
	bool mandelbrotStart();
	bool serverStart();
	void peerAdd();
#if APP_HAS_GLSLANG
	bool mustCompileShader();
	bool compilerStart();
#endif
	void configPrint(ConfigMandelbrot *pMandel);
	void progressPrint();
	void resultPrint();

	void hideCursor();
	void showCursor();

	/* member variables */
	//uint32_t mStartMs;
	uint32_t mStateSd;
	MandelbrotCreating *mpMbCreate;
	TcpListening *mpListen;
	uint32_t mIdxLineDone;
#if APP_HAS_VULKAN
	ShaderCompiling *mpComp;
#endif

	/* static functions */
#if APP_HAS_VULKAN
	static bool vulkanInit();
#endif

	/* static variables */

	/* constants */

};

#endif

