#ifndef VOXTOPNG_LOADER_HPP
#define VOXTOPNG_LOADER_HPP

#include <vector>
#include "common.hpp"

using namespace std;

static_assert(sizeof(char) == sizeof(int8_t), "size of char much be 8 bits"); // just in case

class Loader
{
	public:
		Loader(const vector<char>& data);
		void reset();

		vector<char> 	readNextChars(size_t numBytes);
		char			readNextChar();
		vector<int32_t> readNextInt32s(size_t numInt32s);
		int32_t			readNextInt32();
		string			seekAny(const vector<string>& texts, bool putPointerAfterHit = false);
		string			seek(const string& text, bool putPointerAfterHit = false);

	private:
		const vector<char>& m_data;
		size_t m_pointer = 0u;

		void syncDebugDataLeft();

#ifdef _DEBUG
		struct
		{
			const char* chars = nullptr;
			vector<char> charVec;
			vector<int32_t> ints;
		} m_dataRemaining;
#endif
};


#endif //VOXTOPNG_LOADER_HPP
