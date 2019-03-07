
#ifndef VOXTOPNG_COMMON_HPP
#define VOXTOPNG_COMMON_HPP

#include <cstdint>
#include <string>

using namespace std;

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

#endif //VOXTOPNG_COMMON_HPP
