#pragma once

#define LVL_DEPTH	10

class meanvalue {

	float lvl[LVL_DEPTH];
	int num;

public:
	meanvalue();
	void add(float v);
	float getval();
	float getmaxval();
};