
#include <float.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"

namespace {
		
	unsigned short* boxBlur(rcCompactHeightfield& chf, int thr, unsigned short* src, unsigned short* dst)
	{
		const int w = chf.width;
		const int h = chf.height;

		thr *= 2;

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const rcCompactCell& c = chf.cells[x + y*w];
				for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
				{
					const rcCompactSpan& s = chf.spans[i];
					const unsigned short cd = src[i];
					if (cd <= thr)
					{
						dst[i] = cd;
						continue;
					}

					int d = (int)cd;
					for (int dir = 0; dir < 4; ++dir)
					{
						if (rcGetCon(s, dir) != RC_NOT_CONNECTED)
						{
							const int ax = x + rcGetDirOffsetX(dir);
							const int ay = y + rcGetDirOffsetY(dir);
							const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, dir);
							d += (int)src[ai];

							const rcCompactSpan& as = chf.spans[ai];
							const int dir2 = (dir + 1) & 0x3;
							if (rcGetCon(as, dir2) != RC_NOT_CONNECTED)
							{
								const int ax2 = ax + rcGetDirOffsetX(dir2);
								const int ay2 = ay + rcGetDirOffsetY(dir2);
								const int ai2 = (int)chf.cells[ax2 + ay2*w].index + rcGetCon(as, dir2);
								d += (int)src[ai2];
							}
							else
							{
								d += cd;
							}
						}
						else
						{
							d += cd * 2;
						}
					}
					dst[i] = (unsigned short)((d + 5) / 9);
				}
			}
		}
		return dst;
	}

	void calculateDistanceField(rcCompactHeightfield& chf, unsigned short* src, unsigned short& maxDist)
	{
		const int w = chf.width;
		const int h = chf.height;

		// Init distance and points.
		for (int i = 0; i < chf.spanCount; ++i)
			src[i] = 0xffff;

		// Mark boundary cells.
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const rcCompactCell& c = chf.cells[x + y*w];
				for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
				{
					const rcCompactSpan& s = chf.spans[i];
					const navAreaMask area = chf.areaMasks[i];

					int nc = 0;
					for (int dir = 0; dir < 4; ++dir)
					{
						if (rcGetCon(s, dir) != RC_NOT_CONNECTED)
						{
							const int ax = x + rcGetDirOffsetX(dir);
							const int ay = y + rcGetDirOffsetY(dir);
							const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, dir);
							if (area == chf.areaMasks[ai])
								nc++;
						}
					}
					if (nc != 4)
						src[i] = 0;
				}
			}
		}


		// Pass 1
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const rcCompactCell& c = chf.cells[x + y*w];
				for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
				{
					const rcCompactSpan& s = chf.spans[i];

					if (rcGetCon(s, 0) != RC_NOT_CONNECTED)
					{
						// (-1,0)
						const int ax = x + rcGetDirOffsetX(0);
						const int ay = y + rcGetDirOffsetY(0);
						const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, 0);
						const rcCompactSpan& as = chf.spans[ai];
						if (src[ai] + 2 < src[i])
							src[i] = src[ai] + 2;

						// (-1,-1)
						if (rcGetCon(as, 3) != RC_NOT_CONNECTED)
						{
							const int aax = ax + rcGetDirOffsetX(3);
							const int aay = ay + rcGetDirOffsetY(3);
							const int aai = (int)chf.cells[aax + aay*w].index + rcGetCon(as, 3);
							if (src[aai] + 3 < src[i])
								src[i] = src[aai] + 3;
						}
					}
					if (rcGetCon(s, 3) != RC_NOT_CONNECTED)
					{
						// (0,-1)
						const int ax = x + rcGetDirOffsetX(3);
						const int ay = y + rcGetDirOffsetY(3);
						const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, 3);
						const rcCompactSpan& as = chf.spans[ai];
						if (src[ai] + 2 < src[i])
							src[i] = src[ai] + 2;

						// (1,-1)
						if (rcGetCon(as, 2) != RC_NOT_CONNECTED)
						{
							const int aax = ax + rcGetDirOffsetX(2);
							const int aay = ay + rcGetDirOffsetY(2);
							const int aai = (int)chf.cells[aax + aay*w].index + rcGetCon(as, 2);
							if (src[aai] + 3 < src[i])
								src[i] = src[aai] + 3;
						}
					}
				}
			}
		}

		// Pass 2
		for (int y = h - 1; y >= 0; --y)
		{
			for (int x = w - 1; x >= 0; --x)
			{
				const rcCompactCell& c = chf.cells[x + y*w];
				for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
				{
					const rcCompactSpan& s = chf.spans[i];

					if (rcGetCon(s, 2) != RC_NOT_CONNECTED)
					{
						// (1,0)
						const int ax = x + rcGetDirOffsetX(2);
						const int ay = y + rcGetDirOffsetY(2);
						const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, 2);
						const rcCompactSpan& as = chf.spans[ai];
						if (src[ai] + 2 < src[i])
							src[i] = src[ai] + 2;

						// (1,1)
						if (rcGetCon(as, 1) != RC_NOT_CONNECTED)
						{
							const int aax = ax + rcGetDirOffsetX(1);
							const int aay = ay + rcGetDirOffsetY(1);
							const int aai = (int)chf.cells[aax + aay*w].index + rcGetCon(as, 1);
							if (src[aai] + 3 < src[i])
								src[i] = src[aai] + 3;
						}
					}
					if (rcGetCon(s, 1) != RC_NOT_CONNECTED)
					{
						// (0,1)
						const int ax = x + rcGetDirOffsetX(1);
						const int ay = y + rcGetDirOffsetY(1);
						const int ai = (int)chf.cells[ax + ay*w].index + rcGetCon(s, 1);
						const rcCompactSpan& as = chf.spans[ai];
						if (src[ai] + 2 < src[i])
							src[i] = src[ai] + 2;

						// (-1,1)
						if (rcGetCon(as, 0) != RC_NOT_CONNECTED)
						{
							const int aax = ax + rcGetDirOffsetX(0);
							const int aay = ay + rcGetDirOffsetY(0);
							const int aai = (int)chf.cells[aax + aay*w].index + rcGetCon(as, 0);
							if (src[aai] + 3 < src[i])
								src[i] = src[aai] + 3;
						}
					}
				}
			}
		}

		maxDist = 0;
		for (int i = 0; i < chf.spanCount; ++i)
			maxDist = rcMax(src[i], maxDist);

	}
}

/// @par
///
/// This is just the beginning of the process of fully building a compact heightfield.
/// Various filters may be applied, then the distance field and regions built.
/// E.g: #rcBuildDistanceField and #rcBuildRegions
///
/// See the #rcConfig documentation for more information on the configuration parameters.
///
/// @see rcAllocCompactHeightfield, rcHeightfield, rcCompactHeightfield, rcConfig
bool rcBuildCompactHeightfield(rcContext* ctx, const int walkableHeight, const int walkableClimb, rcHeightfield& hf, rcCompactHeightfield& chf)
{
	rcAssert(ctx);

	ctx->startTimer(RC_TIMER_BUILD_COMPACTHEIGHTFIELD);

	const int w = hf.width;
	const int h = hf.height;
	const int spanCount = rcGetHeightFieldSpanCount(ctx, hf);

	// Fill in header.
	chf.width = w;
	chf.height = h;
	chf.spanCount = spanCount;
	chf.walkableHeight = walkableHeight;
	chf.walkableClimb = walkableClimb;
	chf.maxRegions = 0;
	rcVcopy(chf.bmin, hf.bmin);
	rcVcopy(chf.bmax, hf.bmax);
	chf.bmax[1] += walkableHeight*hf.ch;
	chf.cs = hf.cs;
	chf.ch = hf.ch;
	chf.cells = (rcCompactCell*)rcAlloc(sizeof(rcCompactCell)*w*h, RC_ALLOC_PERM);
	if (!chf.cells)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.cells' (%d)", w*h);
		return false;
	}
	memset(chf.cells, 0, sizeof(rcCompactCell)*w*h);
	chf.spans = (rcCompactSpan*)rcAlloc(sizeof(rcCompactSpan)*spanCount, RC_ALLOC_PERM);
	if (!chf.spans)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.spans' (%d)", spanCount);
		return false;
	}
	memset(chf.spans, 0, sizeof(rcCompactSpan)*spanCount);
	chf.areaMasks = (navAreaMask*)rcAlloc(sizeof(navAreaMask)*spanCount, RC_ALLOC_PERM);
	if (!chf.areaMasks)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.areaMasks' (%d)", spanCount);
		return false;
	}
	memset(chf.areaMasks, RC_NULL_AREA, sizeof(navAreaMask)*spanCount);

	const int MAX_HEIGHT = 0xffff;

	// Fill in cells and spans.
	int idx = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcSpan* s = hf.spans[x + y*w];
			// If there are no spans at this cell, just leave the data to index=0, count=0.
			if (!s) continue;
			rcCompactCell& c = chf.cells[x + y*w];
			c.index = idx;
			c.count = 0;
			while (s)
			{
				if (s->areaMask != RC_NULL_AREA)
				{
					const int bot = (int)s->smax;
					const int top = s->next ? (int)s->next->smin : MAX_HEIGHT;
					chf.spans[idx].minY = (unsigned short)rcClamp(bot, 0, 0xffff);
					chf.spans[idx].height = (unsigned char)rcClamp(top - bot, 0, 0xff);
					chf.areaMasks[idx] = s->areaMask;
					idx++;
					c.count++;
				}
				s = s->next;
			}
		}
	}

	// Find neighbour connections.
	const int MAX_LAYERS = RC_NOT_CONNECTED - 1;
	int tooHighNeighbour = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcCompactCell& c = chf.cells[x + y*w];
			for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
			{
				rcCompactSpan& s = chf.spans[i];

				for (int dir = 0; dir < 4; ++dir)
				{
					rcSetCon(s, dir, RC_NOT_CONNECTED);
					const int nx = x + rcGetDirOffsetX(dir);
					const int ny = y + rcGetDirOffsetY(dir);
					// First check that the neighbour cell is in bounds.
					if (nx < 0 || ny < 0 || nx >= w || ny >= h)
						continue;

					// Iterate over all neighbour spans and check if any of the is
					// accessible from current cell.
					const rcCompactCell& nc = chf.cells[nx + ny*w];
					for (int k = (int)nc.index, nk = (int)(nc.index + nc.count); k < nk; ++k)
					{
						const rcCompactSpan& ns = chf.spans[k];
						const int bot = rcMax(s.minY, ns.minY);
						const int top = rcMin(s.minY + s.height, ns.minY + ns.height);

						// Check that the gap between the spans is walkable,
						// and that the climb height between the gaps is not too high.
						if ((top - bot) >= walkableHeight && rcAbs((int)ns.minY - (int)s.minY) <= walkableClimb)
						{
							// Mark direction as walkable.
							const int lidx = k - (int)nc.index;
							if (lidx < 0 || lidx > MAX_LAYERS)
							{
								tooHighNeighbour = rcMax(tooHighNeighbour, lidx);
								continue;
							}
							rcSetCon(s, dir, lidx);
							break;
						}
					}

				}
			}
		}
	}

	if (tooHighNeighbour > MAX_LAYERS)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Heightfield has too many layers %d (max: %d)", tooHighNeighbour, MAX_LAYERS);
	}

	ctx->stopTimer(RC_TIMER_BUILD_COMPACTHEIGHTFIELD);

	return true;
}

/// @par
/// 
/// This is usually the second to the last step in creating a fully built
/// compact heightfield.  This step is required before regions are built
/// using #rcBuildRegions or #rcBuildRegionsMonotone.
/// 
/// After this step, the distance data is available via the rcCompactHeightfield::maxDistance
/// and rcCompactHeightfield::dist fields.
///
/// @see rcCompactHeightfield, rcBuildRegions, rcBuildRegionsMonotone
bool rcBuildDistanceField(rcContext* ctx, rcCompactHeightfield& chf)
{
	rcAssert(ctx);

	ctx->startTimer(RC_TIMER_BUILD_DISTANCEFIELD);

	if (chf.dist)
	{
		rcFree(chf.dist);
		chf.dist = 0;
	}

	unsigned short* src = (unsigned short*)rcAlloc(sizeof(unsigned short)*chf.spanCount, RC_ALLOC_TEMP);
	if (!src)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildDistanceField: Out of memory 'src' (%d).", chf.spanCount);
		return false;
	}
	unsigned short* dst = (unsigned short*)rcAlloc(sizeof(unsigned short)*chf.spanCount, RC_ALLOC_TEMP);
	if (!dst)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildDistanceField: Out of memory 'dst' (%d).", chf.spanCount);
		rcFree(src);
		return false;
	}

	unsigned short maxDist = 0;

	ctx->startTimer(RC_TIMER_BUILD_DISTANCEFIELD_DIST);

	calculateDistanceField(chf, src, maxDist);
	chf.maxDistance = maxDist;

	ctx->stopTimer(RC_TIMER_BUILD_DISTANCEFIELD_DIST);

	ctx->startTimer(RC_TIMER_BUILD_DISTANCEFIELD_BLUR);

	// Blur
	if (boxBlur(chf, 1, src, dst) != src)
		rcSwap(src, dst);

	// Store distance.
	chf.dist = src;

	ctx->stopTimer(RC_TIMER_BUILD_DISTANCEFIELD_BLUR);

	ctx->stopTimer(RC_TIMER_BUILD_DISTANCEFIELD);

	rcFree(dst);

	return true;
}