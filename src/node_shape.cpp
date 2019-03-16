
#include <climits>
#include <assert.h>
#include "node_shape.hpp"
#include "loader.hpp"
#include "scene.hpp"
#include "model.hpp"

NodeShapeModel::NodeShapeModel(const Context& context)
{
	context.pScene->registerShape(this);
};

int32_t NodeShapeModel::getId() const
{
	return m_id;
}

error NodeShapeModel::load(Loader& loader)
{
	m_id = loader.readNextInt32();
	error err = m_attributes.load(loader);
	if(err != "")
	{
		return err;
	}
	return "";
}

const weak_ptr<Model>& NodeShapeModel::getModel() const
{
	return m_pModel;
}

void NodeShapeModel::setModel(const weak_ptr<Model>& pModel)
{
	m_pModel = pModel;
}

NodeShape::NodeShape(const Context& context)
	: Node(context)
{
	m_cache.isDirty = true;
}

error NodeShape::load(Loader& loader)
{
	error err = Node::load(loader);
	if(err != "")
	{
		return err;
	}

	int32_t numModels = loader.readNextInt32();
	m_models.reserve((size_t)numModels);
	for(size_t i=0; i<numModels; i++)
	{
		shared_ptr<NodeShapeModel> model(new NodeShapeModel(getContext()));
		err = model->load(loader);
		if(err != "")
		{
			return err;
		}
		m_models.push_back(model);
	}

	return "";
}

const Color* NodeShape::getVoxel(int x, int y, int z) const
{
	for(auto&& modelWeak : m_models)
	{
		shared_ptr<Model> model = modelWeak->getModel().lock();
		const vector<Voxel>& voxels = model->getVoxels();

		for(auto&& voxel : voxels)
		{
			if(voxel.x == x && voxel.y == y && voxel.z == z)
			{
				return &getContext().pScene->lookupPaletteColor(voxel.colorIndex);
			}
		}
	}
	return nullptr;
}

const Color* NodeShape::getVoxelGlobal(int x, int y, int z, int32_t* pModel) const
{
	if(m_cache.isDirty)
	{
		for(auto&& modelWeak : m_models)
		{
			m_cache.globallyTranslatedVoxels.clear();
			CachedGlobalModel cachedModel;
			cachedModel.pModel = &(*modelWeak);
			shared_ptr<Model> model = modelWeak->getModel().lock();
			const vector<Voxel>& voxels = model->getVoxels();
			cachedModel.offsetX = INT_MAX;
			cachedModel.offsetY = INT_MAX;
			cachedModel.offsetZ = INT_MAX;
			int endX = -INT_MAX;
			int endY = -INT_MAX;
			int endZ = -INT_MAX;
			vector<CachedVoxel_32i> globalVoxels;
			for(auto&& voxel : voxels)
			{
				CachedVoxel_32i cachedVoxel;
				transformGlobal(voxel.x, voxel.y, voxel.z, &cachedVoxel.x, &cachedVoxel.y, &cachedVoxel.z, true);
				cachedVoxel.pColor = &getContext().pScene->lookupPaletteColor(voxel.colorIndex);
				globalVoxels.push_back(cachedVoxel);
				cachedModel.offsetX = min(cachedModel.offsetX, cachedVoxel.x);
				cachedModel.offsetY = min(cachedModel.offsetY, cachedVoxel.y);
				cachedModel.offsetZ = min(cachedModel.offsetZ, cachedVoxel.z);
				endX = max(endX, cachedVoxel.x);
				endY = max(endY, cachedVoxel.y);
				endZ = max(endZ, cachedVoxel.z);
			}

			cachedModel.width = (uint)((endX - cachedModel.offsetX)+1);
			cachedModel.height = (uint)((endY - cachedModel.offsetY)+1);
			cachedModel.depth = (uint)((endZ - cachedModel.offsetZ)+1);

			cachedModel.globalVoxels.resize((cachedModel.width+1)*(cachedModel.height+1)*(cachedModel.depth+1));

			for(auto&& globalVoxel : globalVoxels)
			{
				int localX = globalVoxel.x - cachedModel.offsetX;
				int localY = globalVoxel.y - cachedModel.offsetY;
				int localZ = globalVoxel.z - cachedModel.offsetZ;

				assert(localX >= 0 && localX <= 0xff);
				assert(localY >= 0 && localY <= 0xff);
				assert(localZ >= 0 && localZ <= 0xff);

				size_t targetIndex = localX + (cachedModel.width * localY) + (cachedModel.width * cachedModel.height * localZ);
				assert(targetIndex < cachedModel.globalVoxels.size());
				for(size_t i=cachedModel.globalVoxels.size(); i<targetIndex+1; i++)
				{
					cachedModel.globalVoxels.emplace_back(CachedVoxel());
				}

				CachedVoxel& target = cachedModel.globalVoxels[targetIndex];
				target.x = globalVoxel.x;
				target.y = globalVoxel.y;
				target.z = globalVoxel.z;
				target.pColor = globalVoxel.pColor;
			}
			m_cache.globallyTranslatedVoxels.push_back(cachedModel);
		}
		m_cache.isDirty = false;
	}

	for(auto&& cached : m_cache.globallyTranslatedVoxels)
	{
		if(pModel == nullptr || cached.pModel->getId() == *pModel)
		{
			if(x >= cached.offsetX && y >= cached.offsetY && z >= cached.offsetZ && x < (cached.offsetX + (int)cached.width) && y < (cached.offsetY + (int)cached.height) && z < (cached.offsetZ + (int)cached.depth))
			{
				int localX = x - cached.offsetX;
				int localY = y - cached.offsetY;
				int localZ = z - cached.offsetZ;
				size_t targetIndex = localX + (cached.width * localY) + (cached.width * cached.height * localZ);
				assert(targetIndex < cached.globalVoxels.size());
				const CachedVoxel& voxel = cached.globalVoxels[targetIndex];
				assert(voxel.pColor == nullptr || (voxel.pColor != nullptr && voxel.x == x && voxel.y == y && voxel.z == z));
				return voxel.pColor;
			}
		}
	}
	return nullptr;
}

void NodeShape::getSize(int32_t* pX, int32_t* pY, int32_t* pZ) const
{
	int32_t width = 0;
	int32_t height = 0;
	int32_t depth = 0;
	for(auto&& model : m_models)
	{
		int32_t modelWidth = 0;
		int32_t modelHeight = 0;
		int32_t modelDepth = 0;
		model->getModel().lock()->getSize(&modelWidth, &modelHeight, &modelDepth);

		width = max(width, modelWidth);
		height = max(height, modelHeight);
		depth = max(depth, modelDepth);
	}
	if(pX != nullptr)
	{
		*pX = width;
	}
	if(pY != nullptr)
	{
		*pY = height;
	}
	if(pZ != nullptr)
	{
		*pZ = depth;
	}
}

bool NodeShape::hasModelId(int32_t id) const
{
	for(auto&& model : m_models)
	{
		if(model->getId() == id)
		{
			return true;
		}
	}
	return false;
}

void NodeShape::getGlobalBounds(int32_t* pX0, int32_t* pY0, int32_t* pZ0, int32_t* pX1, int32_t* pY1, int32_t* pZ1) const
{
	int x0 = INT32_MAX;
	int y0 = INT32_MAX;
	int z0 = INT32_MAX;
	int x1 = -INT32_MAX;
	int y1 = -INT32_MAX;
	int z1 = -INT32_MAX;

	getVoxelGlobal(0, 0, 0, nullptr);

	for(auto&& cached : m_cache.globallyTranslatedVoxels)
	{
		x0 = min(cached.offsetX, x0);
		x1 = max(cached.offsetX, x1);
		x0 = min(cached.offsetX + (int)cached.width, x0);
		x1 = max(cached.offsetX + (int)cached.width, x1);
		y0 = min(cached.offsetY, y0);
		y1 = max(cached.offsetY, y1);
		y0 = min(cached.offsetY + (int)cached.height, y0);
		y1 = max(cached.offsetY + (int)cached.height, y1);
		z0 = min(cached.offsetZ, z0);
		z1 = max(cached.offsetZ, z1);
		z0 = min(cached.offsetZ + (int)cached.depth, z0);
		z1 = max(cached.offsetZ + (int)cached.depth, z1);
	}

	if(pX0 != nullptr)
	{
		*pX0 = x0;
	}
	if(pY0 != nullptr)
	{
		*pY0 = y0;
	}
	if(pZ0 != nullptr)
	{
		*pZ0 = z0;
	}
	if(pX1 != nullptr)
	{
		*pX1 = x1;
	}
	if(pY1 != nullptr)
	{
		*pY1 = y1;
	}
	if(pZ1 != nullptr)
	{
		*pZ1 = z1;
	}
}

void NodeShape::makeDirty()
{
	Node::makeDirty();
	m_cache.isDirty = true;
}
