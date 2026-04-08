/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 25.03.2026

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

#ifndef LIB_MANDEL_H
#define LIB_MANDEL_H

#include <stddef.h>
#include <inttypes.h>
#include <string>

typedef double MbValFull;
typedef float MbVal;

const size_t cShiftElem = 2;
const size_t cNumDoublesPerBlock = 1 << cShiftElem;
const size_t cMaskElem = cNumDoublesPerBlock - 1;

typedef int8_t ElemColor;

class Color
{
public:
	Color(uint8_t r_ = 0, uint8_t g_ = 0, uint8_t b_ = 0)
		: mR((ElemColor)r_)
		, mG((ElemColor)g_)
		, mB((ElemColor)b_)
	{}

	uint8_t r() { return mR; }
	uint8_t g() { return mG; }
	uint8_t b() { return mB; }

	Color operator+(const Color &other) const
	{ return Color(mR + other.mR, mG + other.mG, mB + other.mB); }
	Color operator-(const Color &other) const
	{ return Color(mR - other.mR, mG - other.mG, mB - other.mB); }
	Color operator*(MbValFull t) const
	{ return Color(mR * t, mG * t, mB * t); }

	ElemColor mR;
	ElemColor mG;
	ElemColor mB;
};

struct GradientStop
{
	MbValFull t;
	Color c;
};

struct ConfigMandelbrot
{
	// Image
	uint32_t imgWidth;
	uint32_t imgHeight;
	size_t szData;
	size_t szLine;
	size_t szPadding;
	std::string nameFile;
	std::string dirOut;

	// Mandelbrot
	bool forceDouble;
	bool useDouble;
#if APP_HAS_AVX2
	bool disableSimd;
#endif
#if APP_HAS_VULKAN
	bool disableGpu;
#endif
	size_t numIterMax;
	MbValFull posX;
	MbValFull posY;
	MbValFull zoom;

	MbValFull w2;
	MbValFull h2;
	MbValFull scaleX;
	MbValFull scaleY;

	// Filling
	size_t numBurst;
};

void libMandelInit();
size_t colorMandelbrotScalar(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel);
#if APP_HAS_AVX2
size_t colorMandelbrotSimd(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel);
#endif
void gradientsGet(GradientStop * &pStart, size_t &numElements);

#endif

