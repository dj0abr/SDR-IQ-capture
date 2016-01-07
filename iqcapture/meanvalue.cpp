// Calculate the mean value of a number of values

#include "iqcapture.h"

meanvalue::meanvalue()
{
	num = 0;
	for (int i = 0; i < LVL_DEPTH; i++) lvl[i] = 0;
}

void meanvalue::add(float v)
{
	v = fabs(v);

	for (int i = (LVL_DEPTH - 1); i >= 1; i--)
	{
		lvl[i] = lvl[i - 1];
	}
	lvl[0] = v;

	// initialize an empty array with the first measured value
	if (num == 0)
	{
		num = 1;
		for (int i = 1; i < LVL_DEPTH; i++) lvl[i] = v;
	}
}

float meanvalue::getval()
{
	float m = 0;
	for (int i = 0; i < LVL_DEPTH; i++)
	{
		m += lvl[i];
	}
	return m / LVL_DEPTH;
}

float meanvalue::getmaxval()
{
	float m = 0;
	for (int i = 0; i < LVL_DEPTH; i++)
	{
		if (lvl[i] > m) m = lvl[i];
	}
	return m;
}
