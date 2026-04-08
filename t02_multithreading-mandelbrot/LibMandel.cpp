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

#include <math.h>
#if APP_HAS_AVX2
#include <immintrin.h>
#endif

#include "LibMandel.h"
#include "LibDspc.h"

using namespace std;

static GradientStop keysGradient[] =
{
	{0.00,  {  0,   0,   0}}, // black
	{0.05,  {  0,   0,  80}}, // deep blue
	{0.10,  {  0,   0, 150}}, // blue
	{0.15,  {  0,  50, 200}}, // blue-cyan
	{0.20,  {  0, 120, 220}}, // cyan
	{0.25,  { 40, 180, 255}}, // light cyan
	{0.30,  {120, 220, 255}}, // very light blue
	{0.35,  {200, 240, 255}}, // almost white
	{0.40,  {255, 255, 255}}, // white
	{0.45,  {255, 240, 180}}, // warm white
	{0.50,  {255, 220, 120}}, // light gold
	{0.55,  {255, 200,  60}}, // gold
	{0.60,  {255, 170,   0}}, // deep gold
	{0.65,  {227, 145,   0}},
	{0.70,  {200, 120,   0}}, // bronze
	{0.75,  {160,  90,   0}},
	{0.80,  {120,  60,   0}}, // dark bronze
	{0.85,  { 90,  25,   0}},
	{0.90,  { 60,  30,   0}}, // dark brown
	{0.95,  { 30,  15,   0}},
	{1.00,  {  0,   0,   0}}, // back to black
};

const size_t cScaleGradient = 20;

const size_t cNumKeysGradient = sizeof(keysGradient) / sizeof(keysGradient[0]);
const size_t cNumGradients = (cNumKeysGradient - 1) * cScaleGradient + 1;

static GradientStop gradient[cNumGradients] = {};

#if APP_HAS_AVX2
static __m256d cOne, cTwo, cFour;
static __m256 cOneF, cTwoF, cFourF;
#endif

#define THIS_DEBUG 0

template<typename T>
static T fractionalIter(
			T zx, T zy,
			size_t numIter)
{
	T mag = sqrt(zx * zx + zy * zy);
	return numIter + 1 - log2(log2(mag));
}

template<typename T>
static void mandelbrot(
			T cx, T cy, size_t numIterMax,
			T &zxOut, T &zyOut, size_t &numIterOut)
{
	T xx, yy, xy, zx, zy;
	size_t numIter;

	zx = 0.0;
	zy = 0.0;

	numIter = 0;

	while (numIter < numIterMax)
	{
		xx = zx * zx;
		yy = zy * zy;

		if (xx + yy > 4.0)
			break;

		xy = zx * zy;

		zx = xx - yy + cx;
		zy = 2 * xy + cy;

		++numIter;
	}

	zxOut = zx;
	zyOut = zy;
	numIterOut = numIter;
}

// (x, y) -> (r, g, b)
template<typename T>
static void colorMandelbrotScalar(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel, size_t &numIter)
{
	// 1. From image pixel space -> Complex space

	T sxf = pCfg->scaleX;
	T syf = pCfg->scaleY;
	T pxf = pCfg->posX;
	T pyf = pCfg->posY;
	T w2f = pCfg->w2;
	T h2f = pCfg->h2;

	T idxX = idxPixel - w2f;
	T idxY = idxLine - h2f;
	T cx = sxf * idxX + pxf;
	T cy = syf * idxY + pyf;

	// 2. Do the mandelbrot calculation in complex space

	size_t numIterMax = pCfg->numIterMax;
	T zx, zy;

	mandelbrot<T>(cx, cy, numIterMax, zx, zy, numIter);

	// 3. Color mapping from fractional iterator -> RGB color

	if (numIter >= numIterMax)
	{
		*pData++ = 0;
		*pData++ = 0;
		*pData++ = 0;

		return;
	}

	GradientStop *pGrad1, *pGrad2;
	T mu, t, tMin, tMax;
	size_t idxGrad1;
	Color c;

	mu = fractionalIter<T>(zx, zy, numIter);

	t = mu * (T)0.02;
	t = t - floor(t);

	tMin = 0.0;
	tMax = 1.0;

	t = PMAX(tMin, PMIN(tMax, t));

	idxGrad1 = (size_t)(t * (cNumGradients - 1));
	idxGrad1 = PMIN(idxGrad1, cNumGradients - 2);

	pGrad1 = &gradient[idxGrad1];
	pGrad2 = pGrad1 + 1;

	c = lerp(t, pGrad1->c, pGrad2->c);
#if THIS_DEBUG
	dbgLog("-----------------------------------");
	dbgLog("idxX            %12zu", idxPixel);
	dbgLog("idxY            %12zu", idxLine);
	dbgLog("cx              %12.8f", cx);
	dbgLog("cy              %12.8f", cy);
	dbgLog("zx              %12.8f", zx);
	dbgLog("zy              %12.8f", zy);
	dbgLog("numIter         %12u", numIter);
	dbgLog("mu              %12.8f", mu);
	dbgLog("t               %12.8f", t);
	dbgLog("idxGrad1        %12u", idxGrad1);
	dbgLog("R/G/B            %3u/%3u/%3u", c.r(), c.g(), c.b());
#endif
	// Not RGB but BGR! => BMP specific
	*pData++ = c.b();
	*pData++ = c.g();
	*pData++ = c.r();
#if 0
	hexDump(pData - 3, 3, "COLOR SCALAR");
#endif
}

size_t colorMandelbrotScalar(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel)
{
	size_t numIter;

	if (pCfg->useDouble)
		colorMandelbrotScalar<MbValFull>(pCfg, pData, idxLine, idxPixel, numIter);
	else
		colorMandelbrotScalar<MbVal>(pCfg, pData, idxLine, idxPixel, numIter);

	return numIter;
}

// (x[4], y[4]) -> (r, g, b)[4]
/*
 * Literature
 * - https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
 */
#if APP_HAS_AVX2
#if THIS_DEBUG
static void m128iPrint(__m128i &val, const char *pName = NULL)
{
	int32_t valOut[cNumDoublesPerBlock];

	_mm_storeu_si128((__m128i *)valOut, val);

	dbgLog("%s = [%d, %d, %d, %d]",
		pName ? pName : "m128i",
		valOut[0], valOut[1], valOut[2], valOut[3]);

	//hexDump(&valOut, sizeof(valOut));
}

static void m256iPrint(__m256i &val, const char *pName = NULL)
{
	int32_t valOut[2 * cNumDoublesPerBlock];

	_mm256_storeu_si256((__m256i *)valOut, val);

	dbgLog("%s = [%d, %d, %d, %d, %d, %d, %d, %d]",
		pName ? pName : "m256i",
		valOut[0], valOut[1], valOut[2], valOut[3],
		valOut[4], valOut[5], valOut[6], valOut[7]);

	//hexDump(&valOut, sizeof(valOut));
}

static void m256Print(__m256 &val, const char *pName = NULL)
{
	MbVal valOut[2 * cNumDoublesPerBlock];

	_mm256_storeu_ps(valOut, val);

	dbgLog("%s = [%.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f]",
		pName ? pName : "m256",
		valOut[0], valOut[1], valOut[2], valOut[3],
		valOut[4], valOut[5], valOut[6], valOut[7]);

	//hexDump(&valOut, sizeof(valOut));
}

static void m256dPrint(__m256d &val, const char *pName = NULL)
{
	MbValFull valOut[cNumDoublesPerBlock];

	_mm256_storeu_pd(valOut, val);

	dbgLog("%s = [%.8f, %.8f, %.8f, %.8f]",
		pName ? pName : "m256d",
		valOut[0], valOut[1], valOut[2], valOut[3]);

	//hexDump(&valOut, sizeof(valOut));
}
#endif
__m128i lerp(MbValFull t_d, __m256d a, __m256d b)
{
	__m256d t, tmp_d;
	__m128i tmp_i;

	t = _mm256_set1_pd(t_d);

	tmp_d = _mm256_sub_pd(b, a);
	tmp_d = _mm256_mul_pd(tmp_d, t);
	tmp_i = _mm256_cvttpd_epi32(tmp_d);

	tmp_i = _mm_add_epi32(_mm256_cvttpd_epi32(a), tmp_i);

	return tmp_i;
}

static __m256 fractionalIter(
			__m256 zx, __m256 zy,
			__m256 numIter)
{
	MbVal mag_f[2 * cNumDoublesPerBlock];
	__m256 xx, yy, mag;

	xx = _mm256_mul_ps(zx, zx);
	yy = _mm256_mul_ps(zy, zy);
	mag = _mm256_sqrt_ps(_mm256_add_ps(xx, yy));

	_mm256_storeu_ps(mag_f, mag);

	for (size_t i = 0; i < 2 * cNumDoublesPerBlock; ++i)
		mag_f[i] = log2(log2(mag_f[i]));

	mag = _mm256_loadu_ps(mag_f);
	mag = _mm256_sub_ps(mag, cOneF);

	return _mm256_sub_ps(numIter, mag);
}

static __m256d fractionalIter(
			__m256d zx, __m256d zy,
			__m256d numIter)
{
	MbValFull mag_d[cNumDoublesPerBlock];
	__m256d xx, yy, mag;

	xx = _mm256_mul_pd(zx, zx);
	yy = _mm256_mul_pd(zy, zy);
	mag = _mm256_sqrt_pd(_mm256_add_pd(xx, yy));

	_mm256_storeu_pd(mag_d, mag);

	for (size_t i = 0; i < cNumDoublesPerBlock; ++i)
		mag_d[i] = log2(log2(mag_d[i]));

	mag = _mm256_loadu_pd(mag_d);
	mag = _mm256_sub_pd(mag, cOne);

	return _mm256_sub_pd(numIter, mag);
}

static void mandelbrot(
			__m256 cx, __m256 cy, size_t numIterMax,
			__m256 &zxOut, __m256 &zyOut, __m256 &numIterOut)
{
	__m256 xx, yy, xy, zx, zy;
	__m256 tmp_f, mask;
	__m256 numIter;

	zx = _mm256_setzero_ps();
	zy = _mm256_setzero_ps();

	numIter = _mm256_setzero_ps();

	for (size_t i = 0; i < numIterMax; ++i)
	{
		xx = _mm256_mul_ps(zx, zx);
		yy = _mm256_mul_ps(zy, zy);

		tmp_f = _mm256_add_ps(xx, yy);
		mask = _mm256_cmp_ps(tmp_f, cFourF, _CMP_LE_OS);

		if (_mm256_testz_ps(mask, mask))
			break;

		xy = _mm256_mul_ps(zx, zy);

		// shoutout for nite_child
		tmp_f = _mm256_add_ps(_mm256_sub_ps(xx, yy), cx);
		zx = _mm256_blendv_ps(zx, tmp_f, mask);

		tmp_f = _mm256_add_ps(_mm256_mul_ps(cTwoF, xy), cy);
		zy = _mm256_blendv_ps(zy, tmp_f, mask);

		tmp_f = _mm256_add_ps(cOneF, numIter);
		numIter = _mm256_blendv_ps(numIter, tmp_f, mask);
	}

	zxOut = zx;
	zyOut = zy;
	numIterOut = numIter;
}

static void mandelbrot(
			__m256d cx, __m256d cy, size_t numIterMax,
			__m256d &zxOut, __m256d &zyOut, __m256d &numIterOut)
{
	__m256d xx, yy, xy, zx, zy;
	__m256d tmp_d, mask;
	__m256d numIter;

	zx = _mm256_setzero_pd();
	zy = _mm256_setzero_pd();

	numIter = _mm256_setzero_pd();

	for (size_t i = 0; i < numIterMax; ++i)
	{
		xx = _mm256_mul_pd(zx, zx);
		yy = _mm256_mul_pd(zy, zy);

		tmp_d = _mm256_add_pd(xx, yy);
		mask = _mm256_cmp_pd(tmp_d, cFour, _CMP_LE_OS);

		if (_mm256_testz_pd(mask, mask))
			break;

		xy = _mm256_mul_pd(zx, zy);

		tmp_d = _mm256_add_pd(_mm256_sub_pd(xx, yy), cx);
		zx = _mm256_blendv_pd(zx, tmp_d, mask);

		tmp_d = _mm256_add_pd(_mm256_mul_pd(cTwo, xy), cy); // _mm256_fmadd_pd
		zy = _mm256_blendv_pd(zy, tmp_d, mask);

		tmp_d = _mm256_add_pd(cOne, numIter);
		numIter = _mm256_blendv_pd(numIter, tmp_d, mask);
	}

	zxOut = zx;
	zyOut = zy;
	numIterOut = numIter;
}

void colorMandelbrotSimdFloat(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel, size_t &numIterSum)
{
	// 1. From image pixel space -> Complex space

	__m256i tmp_i, idxXin, idxYin;
	__m256 tmp_f, idxX, idxY, cx, cy;
	MbVal sxf = pCfg->scaleX;
	MbVal syf = pCfg->scaleY;
	MbVal pxf = pCfg->posX;
	MbVal pyf = pCfg->posY;
	MbVal w2f = pCfg->w2;
	MbVal h2f = pCfg->h2;

	idxXin = _mm256_set_epi32(
			idxPixel + 7, idxPixel + 6, idxPixel + 5, idxPixel + 4,
			idxPixel + 3, idxPixel + 2, idxPixel + 1, idxPixel + 0);
	idxYin = _mm256_set1_epi32(idxLine);

	idxX = _mm256_sub_ps(_mm256_cvtepi32_ps(idxXin), _mm256_set1_ps(w2f));
	idxY = _mm256_sub_ps(_mm256_cvtepi32_ps(idxYin), _mm256_set1_ps(h2f));

	cx = _mm256_mul_ps(_mm256_set1_ps(sxf), idxX);
	cx = _mm256_add_ps(cx, _mm256_set1_ps(pxf));

	cy = _mm256_mul_ps(_mm256_set1_ps(syf), idxY);
	cy = _mm256_add_ps(cy, _mm256_set1_ps(pyf));

	// 2. Do the mandelbrot calculation in complex space

	size_t numIterMax = pCfg->numIterMax;
	MbVal numIter_f[2 * cNumDoublesPerBlock];
	__m256 zx, zy, numIter;

	mandelbrot(cx, cy, numIterMax, zx, zy, numIter);

	_mm256_storeu_ps(numIter_f, numIter);

	numIterSum = 0;
	for (size_t i = 0; i < 2 * cNumDoublesPerBlock; ++i)
		numIterSum += (size_t)numIter_f[i];

	// 3. Color mapping from fractional iterator -> RGB color

	uint32_t idxGrad1_u[2 * cNumDoublesPerBlock];
	MbVal t_f[2 * cNumDoublesPerBlock];
	GradientStop *pGrad1, *pGrad2;
	__m256 mu, t, tMin, tMax;
	__m256i idxGrad1;
	__m128i c;
	__m256d c1, c2;

	mu = fractionalIter(zx, zy, numIter);

	t = _mm256_mul_ps(_mm256_set1_ps(0.02f), mu);
	t = _mm256_sub_ps(t, _mm256_floor_ps(t));

	tMin = _mm256_set1_ps(0.0f);
	tMax = _mm256_set1_ps(1.0f);

	t = _mm256_min_ps(t, tMax);
	t = _mm256_max_ps(t, tMin);

	tmp_i = _mm256_set1_epi32(cNumGradients - 1);
	tmp_f = _mm256_cvtepi32_ps(tmp_i);
	tmp_f = _mm256_mul_ps(t, tmp_f);
	idxGrad1 = _mm256_cvttps_epi32(tmp_f);

	tmp_i = _mm256_set1_epi32(cNumGradients - 2);
	idxGrad1 = _mm256_min_epi32(idxGrad1, tmp_i);

	_mm256_storeu_ps(t_f, t);
	_mm256_storeu_si256((__m256i *)idxGrad1_u, idxGrad1);
#if THIS_DEBUG
	m256iPrint(idxXin, "idxXin");
	m256iPrint(idxYin, "idxYin");
	m256Print(cx, "cx");
	m256Print(cy, "cy");
	m256Print(zx, "zx");
	m256Print(zy, "zy");
	m256Print(numIter, "numIter");
	dbgLog("numIterSum = %zu", numIterSum);
	m256Print(mu, "mu");
	m256Print(t, "t");
	m256iPrint(idxGrad1, "idxGrad1");
#endif
	for (size_t i = 0; i < 2 * cNumDoublesPerBlock; ++i)
	{
		pGrad1 = &gradient[idxGrad1_u[i]];
		pGrad2 = pGrad1 + 1;

		c1 = _mm256_set_pd(0, pGrad1->c.b(), pGrad1->c.g(), pGrad1->c.r());
		c2 = _mm256_set_pd(0, pGrad2->c.b(), pGrad2->c.g(), pGrad2->c.r());

		c = lerp((MbValFull)t_f[i], c1, c2);

		if ((size_t)numIter_f[i] >= numIterMax)
			c = _mm_set1_epi32(0);
#if THIS_DEBUG
		dbgLog("R/G/B            %3u/%3u/%3u",
				_mm_extract_epi8(c, 0),
				_mm_extract_epi8(c, 4),
				_mm_extract_epi8(c, 8));
#endif
		// Not RGB but BGR! => BMP specific
		*pData++ = _mm_extract_epi8(c, 8);
		*pData++ = _mm_extract_epi8(c, 4);
		*pData++ = _mm_extract_epi8(c, 0);
	}
#if 0
	hexDump(pData - 24, 24, "COLOR SIMD");
#endif
}

void colorMandelbrotSimdDouble(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel, size_t &numIterSum)
{
	// 1. From image pixel space -> Complex space

	__m128i tmp_i, idxXin, idxYin;
	__m256d tmp_d, idxX, idxY, cx, cy;

	idxXin = _mm_set_epi32(idxPixel + 3, idxPixel + 2, idxPixel + 1, idxPixel + 0);
	idxYin = _mm_set1_epi32(idxLine);

	idxX = _mm256_sub_pd(_mm256_cvtepi32_pd(idxXin), _mm256_set1_pd(pCfg->w2));
	idxY = _mm256_sub_pd(_mm256_cvtepi32_pd(idxYin), _mm256_set1_pd(pCfg->h2));

	cx = _mm256_mul_pd(_mm256_set1_pd(pCfg->scaleX), idxX);
	cx = _mm256_add_pd(cx, _mm256_set1_pd(pCfg->posX));

	cy = _mm256_mul_pd(_mm256_set1_pd(pCfg->scaleY), idxY);
	cy = _mm256_add_pd(cy, _mm256_set1_pd(pCfg->posY));

	// 2. Do the mandelbrot calculation in complex space

	size_t numIterMax = pCfg->numIterMax;
	MbValFull numIter_d[cNumDoublesPerBlock];
	__m256d zx, zy, numIter;

	mandelbrot(cx, cy, numIterMax, zx, zy, numIter);

	_mm256_storeu_pd(numIter_d, numIter);

	numIterSum = 0;
	for (size_t i = 0; i < cNumDoublesPerBlock; ++i)
		numIterSum += (size_t)numIter_d[i];

	// 3. Color mapping from fractional iterator -> RGB color

	uint32_t idxGrad1_u[cNumDoublesPerBlock];
	MbValFull t_d[cNumDoublesPerBlock];
	GradientStop *pGrad1, *pGrad2;
	__m256d mu, t, tMin, tMax;
	__m128i idxGrad1, c;
	__m256d c1, c2;

	mu = fractionalIter(zx, zy, numIter);

	t = _mm256_mul_pd(_mm256_set1_pd(0.02), mu);
	t = _mm256_sub_pd(t, _mm256_floor_pd(t));

	tMin = _mm256_set1_pd(0.0);
	tMax = _mm256_set1_pd(1.0);

	t = _mm256_min_pd(t, tMax);
	t = _mm256_max_pd(t, tMin);

	tmp_i = _mm_set1_epi32(cNumGradients - 1);
	tmp_d = _mm256_cvtepi32_pd(tmp_i);
	tmp_d = _mm256_mul_pd(t, tmp_d);
	idxGrad1 = _mm256_cvttpd_epi32(tmp_d);

	tmp_i = _mm_set1_epi32(cNumGradients - 2);
	idxGrad1 = _mm_min_epi32(idxGrad1, tmp_i);

	_mm256_storeu_pd(t_d, t);
	_mm_storeu_si128((__m128i *)idxGrad1_u, idxGrad1);
#if THIS_DEBUG
	m128iPrint(idxXin, "idxXin");
	m128iPrint(idxYin, "idxYin");
	m256dPrint(cx, "cx");
	m256dPrint(cy, "cy");
	m256dPrint(zx, "zx");
	m256dPrint(zy, "zy");
	m256dPrint(numIter, "numIter");
	dbgLog("numIterSum = %zu", numIterSum);
	m256dPrint(mu, "mu");
	m256dPrint(t, "t");
	m128iPrint(idxGrad1, "idxGrad1");
#endif
	for (size_t i = 0; i < cNumDoublesPerBlock; ++i)
	{
		pGrad1 = &gradient[idxGrad1_u[i]];
		pGrad2 = pGrad1 + 1;

		c1 = _mm256_set_pd(0, pGrad1->c.b(), pGrad1->c.g(), pGrad1->c.r());
		c2 = _mm256_set_pd(0, pGrad2->c.b(), pGrad2->c.g(), pGrad2->c.r());

		c = lerp(t_d[i], c1, c2);

		if ((size_t)numIter_d[i] >= numIterMax)
			c = _mm_set1_epi32(0);
#if THIS_DEBUG
		dbgLog("R/G/B            %3u/%3u/%3u",
				_mm_extract_epi8(c, 0),
				_mm_extract_epi8(c, 4),
				_mm_extract_epi8(c, 8));
#endif
		// Not RGB but BGR! => BMP specific
		*pData++ = _mm_extract_epi8(c, 8);
		*pData++ = _mm_extract_epi8(c, 4);
		*pData++ = _mm_extract_epi8(c, 0);
	}
#if 0
	hexDump(pData - 12, 12, "COLOR SIMD");
#endif
}

size_t colorMandelbrotSimd(const ConfigMandelbrot *pCfg, char *pData, size_t idxLine, size_t idxPixel)
{
	size_t numIter;

	if (pCfg->useDouble)
		colorMandelbrotSimdDouble(pCfg, pData, idxLine, idxPixel, numIter);
	else
		colorMandelbrotSimdFloat(pCfg, pData, idxLine, idxPixel, numIter);

	return numIter;
}
#endif

void libMandelInit()
{
	// SIMD constants
#if APP_HAS_AVX2
	cOne = _mm256_set1_pd(1.0);
	cTwo = _mm256_set1_pd(2.0);
	cFour = _mm256_set1_pd(4.0);
	cOneF = _mm256_set1_ps(1.0f);
	cTwoF = _mm256_set1_ps(2.0f);
	cFourF = _mm256_set1_ps(4.0f);
#endif
	// Gradients
	GradientStop *pKey1, *pKey2, *pGrad;
	size_t i, s, k = 0;
	MbValFull t, tMin, tMax;

	for (; k < cNumKeysGradient - 1; ++k)
	{
		for (s = 0; s < cScaleGradient; ++s)
		{
			pKey1 = &keysGradient[k];
			pKey2 = pKey1 + 1;

			i = k * cScaleGradient + s;
			pGrad = &gradient[i];

			tMin = 0.0;
			tMax = 1.0;

			t = ((MbValFull)s) / cScaleGradient;
			t = PMAX(tMin, PMIN(tMax, t));

			pGrad->t = lerp(t, pKey1->t, pKey2->t);
			pGrad->c = lerp(t, pKey1->c, pKey2->c);
#if 0
			if (i >= 32) continue;
			dbgLog("%2u - %2u - %2u: %0.3f, %3u %3u %3u",
				k, s, i, pGrad->t, pGrad->c.r, pGrad->c.g, pGrad->c.b);
#endif
		}
	}

	gradient[cNumGradients - 1] = keysGradient[cNumKeysGradient - 1];
}

void gradientsGet(GradientStop * &pStart, size_t &numElements)
{
	pStart = gradient;
	numElements = cNumGradients;
}

