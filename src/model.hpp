
#ifndef VOXTOPNG_MODEL_HPP
#define VOXTOPNG_MODEL_HPP

#include <vector>
#include "common.hpp"

class Loader;

struct Voxel
{
	int16_t x = 0;
	int16_t y = 0;
	int16_t z = 0;
	uint8_t colorIndex = 0;
};

class Model
{
	public:
		explicit Model(int32_t id);
		error 	loadSize(Loader& loader);
		error 	loadData(Loader& loader);
		int32_t	getId() const;

		void 	getSize(int32_t* pX, int32_t* pY, int32_t* pZ) const;
		const vector<Voxel>& getVoxels() const;

		void 	doubleSize();
		void 	recenterOrigins();

	private:
		const int32_t 	m_id;
		int32_t			m_width = 0u;
		int32_t			m_height = 0u; // height is actualy just the y axis in the magickavoxel editor... so not really 'height' :S
		int32_t			m_depth = 0u;  // this is the actual height in magickavoxel editor
		vector<Voxel>	m_voxels;
};


#endif //VOXTOPNG_MODEL_HPP
