
#ifndef VOXTOPNG_COMMON_HPP
#define VOXTOPNG_COMMON_HPP

#include <cstdint>
#include <string>

using namespace std;

#define MATRIXARRAY_MUL_TO(_MatA, _MatB, _To){ \
		(_To)[0 ] =	(_MatA)[0 ]*(_MatB)[0 ] + (_MatA)[1 ]*(_MatB)[4 ] + (_MatA)[2 ]*(_MatB)[8 ] + (_MatA)[3 ]*(_MatB)[12]; (_To)[1 ] = (_MatA)[0 ]*(_MatB)[1 ] + (_MatA)[1 ]*(_MatB)[5 ] + (_MatA)[2 ]*(_MatB)[9 ] + (_MatA)[3 ]*(_MatB)[13]; (_To)[2 ] = (_MatA)[0 ]*(_MatB)[2 ] + (_MatA)[1 ]*(_MatB)[6 ] + (_MatA)[2 ]*(_MatB)[10] + (_MatA)[3 ]*(_MatB)[14]; (_To)[3 ] = (_MatA)[0 ]*(_MatB)[3 ] + (_MatA)[1 ]*(_MatB)[7 ] + (_MatA)[2 ]*(_MatB)[11] + (_MatA)[3 ]*(_MatB)[15]; \
		(_To)[4 ] =	(_MatA)[4 ]*(_MatB)[0 ] + (_MatA)[5 ]*(_MatB)[4 ] + (_MatA)[6 ]*(_MatB)[8 ] + (_MatA)[7 ]*(_MatB)[12]; (_To)[5 ] = (_MatA)[4 ]*(_MatB)[1 ] + (_MatA)[5 ]*(_MatB)[5 ] + (_MatA)[6 ]*(_MatB)[9 ] + (_MatA)[7 ]*(_MatB)[13]; (_To)[6 ] = (_MatA)[4 ]*(_MatB)[2 ] + (_MatA)[5 ]*(_MatB)[6 ] + (_MatA)[6 ]*(_MatB)[10] + (_MatA)[7 ]*(_MatB)[14]; (_To)[7 ] = (_MatA)[4 ]*(_MatB)[3 ] + (_MatA)[5 ]*(_MatB)[7 ] + (_MatA)[6 ]*(_MatB)[11] + (_MatA)[7 ]*(_MatB)[15]; \
		(_To)[8 ] =	(_MatA)[8 ]*(_MatB)[0 ] + (_MatA)[9 ]*(_MatB)[4 ] + (_MatA)[10]*(_MatB)[8 ] + (_MatA)[11]*(_MatB)[12]; (_To)[9 ] = (_MatA)[8 ]*(_MatB)[1 ] + (_MatA)[9 ]*(_MatB)[5 ] + (_MatA)[10]*(_MatB)[9 ] + (_MatA)[11]*(_MatB)[13]; (_To)[10] = (_MatA)[8 ]*(_MatB)[2 ] + (_MatA)[9 ]*(_MatB)[6 ] + (_MatA)[10]*(_MatB)[10] + (_MatA)[11]*(_MatB)[14]; (_To)[11] = (_MatA)[8 ]*(_MatB)[3 ] + (_MatA)[9 ]*(_MatB)[7 ] + (_MatA)[10]*(_MatB)[11] + (_MatA)[11]*(_MatB)[15]; \
		(_To)[12] = (_MatA)[12]*(_MatB)[0 ] + (_MatA)[13]*(_MatB)[4 ] + (_MatA)[14]*(_MatB)[8 ] + (_MatA)[15]*(_MatB)[12]; (_To)[13] = (_MatA)[12]*(_MatB)[1 ] + (_MatA)[13]*(_MatB)[5 ] + (_MatA)[14]*(_MatB)[9 ] + (_MatA)[15]*(_MatB)[13]; (_To)[14] = (_MatA)[12]*(_MatB)[2 ] + (_MatA)[13]*(_MatB)[6 ] + (_MatA)[14]*(_MatB)[10] + (_MatA)[15]*(_MatB)[14]; (_To)[15] = (_MatA)[12]*(_MatB)[3 ] + (_MatA)[13]*(_MatB)[7 ] + (_MatA)[14]*(_MatB)[11] + (_MatA)[15]*(_MatB)[15]; }

#define MATRIX_MUL_TO(MatrixAIn, MatrixARows, MatrixAcolumns, MatrixBIn, MatrixBRows, MatrixBcolumns, MatrixOut){ \
		static_assert(MatrixAcolumns == MatrixBRows, "number of rows must equal the number of columns of this matrix"); \
		memset((MatrixOut), 0, sizeof(MatrixOut[0]) * MatrixARows * MatrixBcolumns); \
		for(unsigned int MatARow = 0; MatARow < MatrixARows; MatARow++) for(unsigned int MatBCol = 0; MatBCol < MatrixBcolumns; MatBCol++) for(unsigned int MatACol = 0; MatACol < MatrixAcolumns; MatACol++) \
		(MatrixOut)[MatBCol + MatARow * MatrixBcolumns] += (MatrixAIn)[MatACol + MatARow * MatrixAcolumns] * (MatrixBIn)[MatBCol + MatACol * MatrixBcolumns]; \
		}

typedef string error;

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
static void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if(from.empty())
	{
		return;
	}
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

static bool getBit(uint8_t byte, int position) // position in range 0-7
{
	return ((byte >> position) & 0x1) > 0;
}

static int s_identity[16] = {1, 0, 0, 0,    0, 1, 0, 0,     0, 0, 1, 0,     0, 0, 0, 1};

void inverse3By3Matrix(int pBuffer[9]);

//static bool getBit(uint8_t byte, int position) // position in range 0-7
//{
//	return getBitLeftToRight(byte, 7-position);
//}

#endif //VOXTOPNG_COMMON_HPP
