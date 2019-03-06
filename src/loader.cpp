#include <assert.h>
#include "loader.hpp"

Loader::Loader(const vector<char>& data)
	: m_data(data)
{
	syncDebugDataLeft();
}

void Loader::reset()
{
	m_pointer=0;
	syncDebugDataLeft();
}

vector<char> Loader::readNextChars(size_t numBytes)
{
	vector<char> output;
	output.reserve(numBytes+1);
	for(size_t i=m_pointer; i<(min(m_pointer+numBytes, m_data.size()-1)); i++)
	{
		output.push_back(m_data[i]);
	}
	m_pointer+=numBytes;
	assert(output.size() == numBytes);
	syncDebugDataLeft();
	return output;
}

char Loader::readNextChar()
{
	return readNextChars(1)[0];
}

vector<int32_t> Loader::readNextInt32s(size_t numInt32s)
{
	vector<int32_t> ints;
	vector<char> chars = readNextChars(numInt32s*4);
	char* charPtr = chars.data();
	for(size_t i=0; i<chars.size(); i+=4)
	{
		const int32_t* ptr = (const int32_t*)&charPtr[i];
		ints.push_back(*ptr);
	}
	return ints;
}

int32_t	Loader::readNextInt32()
{
	return readNextInt32s(1)[0];
}

string Loader::seek(const vector<string>& texts, bool putPointerAfterHit)
{
	for(; m_pointer<m_data.size(); m_pointer++)
	{
		for(auto&& text : texts)
		{
			bool broke = false;
			for(size_t i=0; i<text.size(); i++)
			{
				if(m_data[m_pointer + i] != text[i])
				{
					broke = true;
					break;
				}
			}
			if(!broke)
			{
				// found
				if(putPointerAfterHit)
				{
					m_pointer += text.size();
				}
				syncDebugDataLeft();
				return text;
			}
		}
	}
	syncDebugDataLeft();
	return ""s;
}

void Loader::syncDebugDataLeft()
{
#ifdef _DEBUG
	if(m_dataRemaining.chars == nullptr) {
		m_dataRemaining.charVec.reserve(m_data.size());
		m_dataRemaining.ints.reserve(m_data.size());
	}

	m_dataRemaining.chars = &m_data[m_pointer];
	m_dataRemaining.charVec.clear();
	m_dataRemaining.ints.clear();
	for(size_t i=m_pointer; i<m_data.size(); i++) {
		m_dataRemaining.charVec.push_back(m_data[i]);
	}
	for(size_t i=m_pointer; i<m_data.size(); i+=4) {
		const char* charPtr = &m_data[i];
		const int32_t* ptr = (const int32_t*)&charPtr[i];
		m_dataRemaining.ints.push_back(m_data[i]);
	}
#endif
}
