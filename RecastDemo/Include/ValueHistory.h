#ifndef VALUEHISTORY_H
#define VALUEHISTORY_H

#include <string>

using std::string;

class ValueHistory
{
public:
	ValueHistory();

	inline void addSample(const float val)
	{
		// Fills m_samples in reverse order?
		m_hsamples = (m_hsamples + MAX_HISTORY - 1) % MAX_HISTORY;
		m_samples[m_hsamples] = val;
	}

	inline int getSampleCount() const
	{
		return MAX_HISTORY;
	}

	inline float getSample(const int i) const
	{
		return m_samples[(m_hsamples + i) % MAX_HISTORY];
	}

	float getSampleMin() const;
	float getSampleMax() const;
	float getAverage() const;
private:
	static const int MAX_HISTORY = 256;
	float m_samples[MAX_HISTORY];
	int m_hsamples;
};

struct GraphParams
{
	void setRect(int ix, int iy, int iw, int ih, int ipad);
	void setValueRange(float ivmin, float ivmax, int indiv, string iunits);

	int x;
	int y;
	int w;
	int h;
	int pad;
	float vmin;
	float vmax;
	int ndiv;
	string units;
};

#endif // VALUEHISTORY_H
