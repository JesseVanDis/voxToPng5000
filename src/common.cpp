
#include <cstring>
#include <assert.h>
#include "common.hpp"

void inverse3By3Matrix(int pBuffer[9])
{
	int m[3*3];
	int identity[9] = {1,0,0, 0,1,0, 0,0,1};

	memcpy(m, pBuffer, sizeof(m));

	if(memcmp(m, identity, sizeof(m)) == 0)
	{
		return;
	}

	int D, t[9];
	double InvD;
	t[0] = m[4] * m[8] - m[5] * m[7];
	t[1] = m[2] * m[7] - m[1] * m[8];
	t[2] = m[1] * m[5] - m[2] * m[4];
	t[3] = m[5] * m[6] - m[3] * m[8];
	t[4] = m[0] * m[8] - m[2] * m[6];
	t[5] = m[2] * m[3] - m[0] * m[5];
	t[6] = m[3] * m[7] - m[4] * m[6];
	t[7] = m[1] * m[6] - m[0] * m[7];
	t[8] = m[0] * m[4] - m[1] * m[3];
	D = m[0] * t[0] + m[1] * t[3] + m[2] * t[6];
	if(abs(D) <= 0)
	{
		assert(false); // unable to invert
	}
	else
	{
		InvD = 1.0/((double)D);

		pBuffer[0] = (int)InvD*t[0];
		pBuffer[1] = (int)InvD*t[1];
		pBuffer[2] = (int)InvD*t[2];
		pBuffer[3] = (int)InvD*t[3];
		pBuffer[4] = (int)InvD*t[4];
		pBuffer[5] = (int)InvD*t[5];
		pBuffer[6] = (int)InvD*t[6];
		pBuffer[7] = (int)InvD*t[7];
		pBuffer[8] = (int)InvD*t[8];
	}
}

