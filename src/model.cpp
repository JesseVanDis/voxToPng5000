
#include "model.hpp"
#include "loader.hpp"

Model::Model(int32_t id)
	: m_id(id)
{
};

error Model::loadSize(Loader& loader)
{
	__attribute__((unused)) int32_t chunkSize = loader.readNextInt32();
	__attribute__((unused)) int32_t numChildrenChunks = loader.readNextInt32();

	m_width = loader.readNextInt32();
	m_height = loader.readNextInt32();
	m_depth = loader.readNextInt32();

	return ""s;
}

error Model::loadData(Loader& loader)
{
	__attribute__((unused)) int32_t chunkSize = loader.readNextInt32();
	__attribute__((unused)) int32_t numChildrenChunks = loader.readNextInt32();

	m_voxels.resize((size_t)loader.readNextInt32());
	for(auto&& v : m_voxels) {
		v.x = loader.readNextChar();
		v.y = loader.readNextChar();
		v.z = loader.readNextChar();
		v.colorIndex = loader.readNextChar();
		v.colorIndex--;
	}
	return ""s;
}

int32_t	Model::getId() const
{
	return m_id;
}

void Model::getSize(int32_t* pX, int32_t* pY, int32_t* pZ) const
{
	if(pX != nullptr)
	{
		*pX = m_width;
	}
	if(pY != nullptr)
	{
		*pY = m_height;
	}
	if(pZ != nullptr)
	{
		*pZ = m_depth;
	}
}

const vector<Voxel>& Model::getVoxels() const
{
	return m_voxels;
}



