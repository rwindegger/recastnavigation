#include "ValueHistory.h"

#include "imgui.h"
#include <string.h>
#include <stdio.h>
#include <limits>

ValueHistory::ValueHistory()
	: m_hsamples(0)
{
	for (int i = 0; i < MAX_HISTORY; ++i)
		m_samples[i] = 0;
}

float ValueHistory::getSampleMin() const
{
	float val = m_samples[0];
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i] < val)
			val = m_samples[i];
	return val;
}

float ValueHistory::getSampleMax() const
{
	float val = m_samples[0];
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i] > val)
			val = m_samples[i];
	return val;
}

float ValueHistory::getAverage() const
{
	float val = 0;
	for (int i = 0; i < MAX_HISTORY; ++i)
		val += m_samples[i];
	return val / (float)MAX_HISTORY;
}

void GraphParams::setRect(int ix, int iy, int iw, int ih, int ipad)
{
	x = ix;
	y = iy;
	w = iw;
	h = ih;
	pad = ipad;
}

void GraphParams::setValueRange(float ivmin, float ivmax, int indiv, string iunits)
{
	vmin = ivmin;
	vmax = ivmax;
	ndiv = indiv;
	units = iunits;
}
