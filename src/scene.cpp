#include <iostream>
#include <assert.h>
#include <memory>

#include "scene.hpp"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "loader.hpp"
#include "node_transform.hpp"
#include "node_group.hpp"
#include "node_shape.hpp"
#include "model.hpp"
#include "stb_image_write.h"

// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt

Scene::Scene() = default;

error Scene::load(const string& voxFilePath)
{
	error err;

	Context context;
	context.pScene = this;

	// load data
	{
		FILE *voxHandle = fopen(voxFilePath.c_str(), "rb");
		if (!voxHandle)
		{
			return "Couldn't open .vox file, are you sure it exists?"s;
		}
		fseek(voxHandle, 0, SEEK_END);
		auto voxLength = ftell(voxHandle);
		fseek(voxHandle, 0, SEEK_SET);
		m_data.resize((size_t)(voxLength + 1));
		fread(m_data.data(), (size_t)voxLength, 1, voxHandle);
		fclose(voxHandle);
	}
	Loader loader(m_data);

	// models
	int32_t nextModelId = 0;
	bool modelFound;
	do
	{
		modelFound = false;
		string key = loader.seek({"SIZE"}, true);
		if(key != ""s)
		{
			shared_ptr<Model> pModel(new Model(nextModelId++));
			err = pModel->loadSize(loader);
			if(err != ""s)
			{
				return err;
			}

			key = loader.seek({"XYZI"}, true);
			if(key != ""s)
			{
				err = pModel->loadData(loader);
				if(err != ""s)
				{
					return err;
				}
				modelFound = true;
				m_models.push_back(pModel);
			}
			else
			{
				return "'SIZE' token found but was not followed by a 'XYZI' tag"s;
			}
		}
	} while(modelFound);

	// palette
	loader.reset();
	{
		string key = loader.seek({"RGBA"}, true);
		if(key != ""s)
		{
			err = m_palette.load(loader);
			if(err != ""s)
			{
				return err;
			}
		}
	}

	// nodes
	loader.reset();
	bool nodeFound;
	do
	{
		nodeFound = false;
		string key = loader.seek({"nTRN", "nGRP", "nSHP"}, true);
		if(!key.empty())
		{
			nodeFound = true;

			shared_ptr<Node> pNode = nullptr;

			if(key == "nTRN")
			{
				shared_ptr<NodeTransform> pTransform(new NodeTransform(context));
				pNode = pTransform;
			}
			else if(key == "nGRP")
			{
				shared_ptr<NodeGroup> pGroup(new NodeGroup(context));
				pNode = pGroup;
			}
			else if(key == "nSHP")
			{
				shared_ptr<NodeShape> pShape(new NodeShape(context));
				pNode = pShape;
			}
			else
			{
				assert(false); // should not get here
			}

			if(m_pRootNode.use_count() == 0)
			{
				m_pRootNode = pNode;
			}
			m_nodes.push_back(pNode);

			err = pNode->load(loader);
			if(err != ""s)
			{
				return err;
			}
		}
	} while(nodeFound);

	// assign children nodes
	for(auto&& link : m_allNodeChildren)
	{
		for(auto&& node : m_nodes)
		{
			if(node->getId() == link.pChild->getId())
			{
				node->setParent(link.pParent);
				link.pChild->setNodePtr(node);
				break;
			}
		}
	}

	// assign models to nodes
	for(auto&& shape : m_allShapes)
	{
		for(auto&& model : m_models)
		{
			if(shape->getId() == model->getId())
			{
				shape->setModel(model);
				break;
			}
		}
	}

	return ""s;
}

void Scene::registerNodeChild(Node* pParent, NodeChild* pChild)
{
	ChildToLink link;
	link.pParent = pParent;
	link.pChild = pChild;
	m_allNodeChildren.push_back(link);
}

void Scene::registerShape(NodeShapeModel* pShape)
{
	m_allShapes.push_back(pShape);
}

static error savePng(const vector<Color>& pixels, size_t width, size_t height, const string& path)
{
	if (!stbi_write_png(path.c_str(), (int)width, (int)height, 4, pixels.data(), (int)width * 4))
	{
		return "Failed to write Png image"s;
	}
	return ""s;
}

void Scene::getBounds(int32_t* pX0, int32_t* pY0, int32_t* pZ0, int32_t* pX1, int32_t* pY1, int32_t* pZ1)
{
	int x0 = INT32_MAX;
	int y0 = INT32_MAX;
	int z0 = INT32_MAX;
	int x1 = -INT32_MAX;
	int y1 = -INT32_MAX;
	int z1 = -INT32_MAX;
	for(auto&& node : m_nodes)
	{
		if(const NodeShape* pShape = node->toNodeShape())
		{
			int x = 0;
			int y = 0;
			int z = 0;
			node->getGlobalPosition(&x, &y, &z);
			x0 = min(x, x0);
			x1 = max(x, x1);
			y0 = min(y, y0);
			y1 = max(y, y1);
			z0 = min(z, z0);
			z1 = max(z, z1);

			int w = 0;
			int h = 0;
			int d = 0;
			pShape->getSize(&w, &h, &d);

			x0 = min(x + w, x0);
			x1 = max(x + w, x1);
			y0 = min(y + h, y0);
			y1 = max(y + h, y1);
			z0 = min(z + d, z0);
			z1 = max(z + d, z1);
		}
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

const Color* Scene::getVoxel(int x, int y, int z) const
{
	for(auto&& node : m_nodes)
	{
		if(const NodeShape* pShape = node->toNodeShape())
		{
			int nodePosX = 0;
			int nodePosY = 0;
			int nodePosZ = 0;
			pShape->getGlobalPosition(&nodePosX, &nodePosY, &nodePosZ);

			int localX = x - nodePosX;
			int localY = y - nodePosY;
			int localZ = z - nodePosZ;

			if(const Color* pColor = pShape->getVoxel(localX, localY, localZ))
			{
				return pColor;
			}
		}
	}
	return nullptr;
}

const Color& Scene::lookupPaletteColor(uint8_t colorIndex) const
{
	m_palette.lookupColor(colorIndex);
}

error Scene::saveAsPngArray(const string& targetFolderPath)
{
	int x0 = INT32_MAX;
	int y0 = INT32_MAX;
	int z0 = INT32_MAX;
	int x1 = -INT32_MAX;
	int y1 = -INT32_MAX;
	int z1 = -INT32_MAX;

	getBounds(&x0, &y0, &z0, &x1, &y1, &z1);

	const size_t width = (size_t)max(x1-x0, 0);
	const size_t height = (size_t)max(y1-y0, 0);
//	const size_t depth = (size_t)max(z1-z0, 0);

	vector<vector<Color>> imageLayers;
	imageLayers.resize((size_t)max(z1-z0, 0));
	for(int z=z0; z<z1; z++)
	{
		vector<Color>& layer = imageLayers[z - z0];
		layer.resize(width*height);

		size_t i=0;
		for(int y=y0; y<y1; y++)
		{
			for(int x=x0; x<x1; x++)
			{
				if(const Color* color = getVoxel(x, y, z))
				{
					assert(i < layer.size());
					layer[i] = *color;
				}
				i++;
			}
		}
	}

	for(size_t i=0; i<imageLayers.size(); i++)
	{
		vector<Color>& layer = imageLayers[i];
		string imageName = "output_"s;
		imageName += to_string(i);
		imageName += ".png"s;

		string outputPath(targetFolderPath);
		outputPath += "/"s;
		outputPath += imageName;

		if(width == 0 || height == 0)
		{
			return "cannot write an image of width or height 0."s;
		}
		savePng(layer, width, height, outputPath);
	}

	return ""s;
}