
#ifndef VOXTOPNG_PALLETE_HPP
#define VOXTOPNG_PALLETE_HPP

#include <vector>
#include "common.hpp"

class Loader;

struct Color
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	void fromRgba(uint32_t rgba);
};

class Palette
{
	public:
		enum
		{
			NumColors=256
		};

		Palette();
		error 	load(Loader& loader);
		const Color& lookupColor(uint8_t colorIndex) const;

	private:
		Color	m_colors[NumColors];

};


#endif //VOXTOPNG_PALLETE_HPP
