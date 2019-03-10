#include <iostream>
#include <assert.h>
#include <memory>
#include <cstring>

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

	doubleScale();
	recenterOrigins();
	printVoxels();

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

static error savePng(const vector<Color>& pixels, size_t width, size_t height, const string& path, bool flipY)
{
	const vector<Color>* pPixels = &pixels;
	vector<Color> flipped;
	if(flipY)
	{
		flipped.resize(pixels.size());
		for(size_t y=0; y<height; y++)
		{
			memcpy(&flipped[((height-1)-y)*width], &pixels[y*width], sizeof(Color)*width);
		}
		pPixels = &flipped;
	}

	if (!stbi_write_png(path.c_str(), (int)width, (int)height, 4, pPixels->data(), (int)width * 4))
	{
		return "Failed to write Png image"s;
	}
	return ""s;
}

void Scene::getBounds(int32_t* pX0, int32_t* pY0, int32_t* pZ0, int32_t* pX1, int32_t* pY1, int32_t* pZ1) const
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
			int w = 0;
			int h = 0;
			int d = 0;
			pShape->getSize(&w, &h, &d);

			int shapeLocalX0 = -w/2;
			int shapeLocalY0 = -h/2;
			int shapeLocalZ0 = -d/2;
			int shapeLocalX1 = w/2;
			int shapeLocalY1 = h/2;
			int shapeLocalZ1 = d/2;

			int shapeGlobalX0 = 0;
			int shapeGlobalY0 = 0;
			int shapeGlobalZ0 = 0;
			int shapeGlobalX1 = 0;
			int shapeGlobalY1 = 0;
			int shapeGlobalZ1 = 0;

			pShape->transformGlobal(shapeLocalX0, shapeLocalY0, shapeLocalZ0, &shapeGlobalX0, &shapeGlobalY0, &shapeGlobalZ0, false);
			pShape->transformGlobal(shapeLocalX1, shapeLocalY1, shapeLocalZ1, &shapeGlobalX1, &shapeGlobalY1, &shapeGlobalZ1, false);

			x0 = min(shapeGlobalX0, x0);
			x1 = max(shapeGlobalX0, x1);
			x0 = min(shapeGlobalX1, x0);
			x1 = max(shapeGlobalX1, x1);
			y0 = min(shapeGlobalY0, y0);
			y1 = max(shapeGlobalY0, y1);
			y0 = min(shapeGlobalY1, y0);
			y1 = max(shapeGlobalY1, y1);
			z0 = min(shapeGlobalZ0, z0);
			z1 = max(shapeGlobalZ0, z1);
			z0 = min(shapeGlobalZ1, z0);
			z1 = max(shapeGlobalZ1, z1);
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
			if(const Color* pColor = pShape->getVoxelGlobal(x, y, z))
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

string Scene::expandTargetFilePath(const string& targetFilePath, size_t sceneSizeX, size_t sceneSizeY, size_t sceneSizeZ) const
{
	string result = targetFilePath;
	replaceAll(result, "{SIZE_X}", to_string(sceneSizeX));
	replaceAll(result, "{SIZE_Y}", to_string(sceneSizeY));
	replaceAll(result, "{SIZE_Z}", to_string(sceneSizeZ));
	return result;
}

void Scene::doubleScale()
{
	// multiply positions
	for(auto&& node : m_nodes)
	{
		if(const NodeTransform* pTransformConst = node->toNodeTransform())
		{
			NodeTransform* pTransform = const_cast<NodeTransform*>(pTransformConst); // yeah ugly I know. wanna fight ?
			int x=0;
			int y=0;
			int z=0;
			pTransform->getTranslation(&x, &y, &z);
			pTransform->setTranslation(x*2, y*2, z*2);
		}
	}

	// correct origins ( a cube of 3,3,3 should have origina of 1.5, 1.5, 1.5, but has 1,1,1 instead.
	// when multiplying by 2, the size will become 6,6,6 and we can finally use integers to get the correct origin of 3,3,3
	// ( its the whole reason why this 'doubleScale' function exists... )
	for(auto&& node : m_nodes)
	{
		if (const NodeShape* pShape = node->toNodeShape())
		{
			if (const NodeTransform* pParentTransformConst = node->getParent() != nullptr ? node->getParent()->toNodeTransform() : nullptr)
			{
				NodeTransform* pTransform = const_cast<NodeTransform*>(pParentTransformConst); // yeah ugly I know. wanna fight ?
				int32_t w = 0;
				int32_t h = 0;
				int32_t d = 0;
				pShape->getSize(&w, &h, &d);

				int globalTransformation[16];
				pTransform->getGlobalTransformation(globalTransformation);

				int adjustment[16];
				memcpy(adjustment, s_identity, sizeof(adjustment));
				if(w & 1)
				{
					adjustment[3] = 1;
				}
				if(h & 1)
				{
					adjustment[7] = 1;
				}
				if(d & 1)
				{
					adjustment[11] = 1;
				}

				int nodeTransformation[16];
				pTransform->getTransformation(nodeTransformation);
				int multiplied[16];
				MATRIXARRAY_MUL_TO(nodeTransformation, adjustment, multiplied);
				pTransform->setTransformation(multiplied);
			}
		}
	}

	for(auto&& model : m_models)
	{
		model->doubleSize();
	}
}

void Scene::recenterOrigins()
{
	for(auto&& model : m_models)
	{
		model->recenterOrigins();
	}
}

void Scene::printVoxels()
{
	int x0 = INT32_MAX;
	int y0 = INT32_MAX;
	int z0 = INT32_MAX;
	int x1 = -INT32_MAX;
	int y1 = -INT32_MAX;
	int z1 = -INT32_MAX;

	getBounds(&x0, &y0, &z0, &x1, &y1, &z1);

	printf("===== SCENE =====\n");
	printf("  - bounds: (%d, %d, %d) to (%d, %d, %d)\n", x0, y0, z0, x1, y1, z1);
	printf("  - size: %d, %d, %d\n", x1-x0, y1-y0, z1-z0);
	printf("\n");

	for(auto&& node : m_nodes)
	{
		if(const NodeShape* pShape = node->toNodeShape())
		{
			for(auto&& model : m_models)
			{
				int32_t modelId = model->getId();

				if(pShape->hasModelId(model->getId()))
				{
					int32_t modelWidth = 0;
					int32_t modelHeight = 0;
					int32_t modelDepth = 0;
					model->getSize(&modelWidth, &modelHeight, &modelDepth);

					int globalTransformation[16];
					pShape->getGlobalTransformation(globalTransformation);


					printf("===== SHAPE ID: %d | MODEL ID: %d =====\n", pShape->getId(), model->getId());
					printf("  - size: %d, %d, %d\n", modelWidth, modelHeight, modelDepth);
					printf("  - transformation: %2.1d, %2.1d, %2.1d, %2.1d \n", globalTransformation[0], globalTransformation[1], globalTransformation[2], globalTransformation[3]);
					printf("  -                 %2.1d, %2.1d, %2.1d, %2.1d \n", globalTransformation[4], globalTransformation[5], globalTransformation[6], globalTransformation[7]);
					printf("  -                 %2.1d, %2.1d, %2.1d, %2.1d \n", globalTransformation[8], globalTransformation[9], globalTransformation[10], globalTransformation[11]);
					printf("  -                 %2.1d, %2.1d, %2.1d, %2.1d \n", globalTransformation[12], globalTransformation[13], globalTransformation[14], globalTransformation[15]);

					printf("%3.1s| %3.1s| %3.1s| %s \n", "x", "y", "z", "col");

					for(int z=z0; z<z1; z++) for(int y=y0; y<y1; y++) for(int x=x0; x<x1; x++)
					{
						if(const Color* pColor = pShape->getVoxelGlobal(x, y, z, &modelId))
						{
							printf("%3.1d, %3.1d, %3.1d, %#010x (%d, %d, %d) \n", x, y, z, pColor->toRgbaHex(), (int)pColor->r, (int)pColor->g, (int)pColor->b);
						}
					}

					printf("\n");

					break;
				}
			}
		}
	}
}


void Scene::getVoxelsAtCorrectScale(vector<const Color*>* pVoxels, uint* pSceneWidth, uint* pSceneHeight, uint* pSceneDepth) const
{
	int x0 = INT32_MAX;
	int y0 = INT32_MAX;
	int z0 = INT32_MAX;
	int x1 = -INT32_MAX;
	int y1 = -INT32_MAX;
	int z1 = -INT32_MAX;
	getBounds(&x0, &y0, &z0, &x1, &y1, &z1);

	int w = (x1-x0)/2;
	int h = (y1-y0)/2;
	int d = (z1-z0)/2;

	if(pVoxels != nullptr)
	{
		pVoxels->resize((((size_t)w)*((size_t)h)*((size_t)d)), nullptr);

		for(auto&& node : m_nodes)
		{
			if(const NodeShape* pShape = node->toNodeShape())
			{
				for(auto&& model : m_models)
				{
					int32_t modelId = model->getId();
					if(pShape->hasModelId(model->getId()))
					{
						int z=z0;
						size_t zi=0u;
						for(; z<z1; z+=2)
						{
							int y=y0;
							size_t yi=0;
							for(; y<y1; y+=2)
							{
								int x=x0;
								size_t xi=0;
								for(; x<x1; x+=2)
								{
									if(const Color* pColor = pShape->getVoxelGlobal(x, y, z, &modelId))
									{
										size_t t = xi+(yi*w)+(zi*h*w);
										assert(t < pVoxels->size());
										(*pVoxels)[t] = pColor;
									}
									xi++;
								}
								yi++;
							}
							zi++;
						}
						break;
					}
				}
			}
		}
	}

	if(pSceneWidth != nullptr)
	{
		*pSceneWidth = (uint)w;
	}
	if(pSceneHeight != nullptr)
	{
		*pSceneHeight = (uint)h;
	}
	if(pSceneDepth != nullptr)
	{
		*pSceneDepth = (uint)d;
	}
}

void Scene::fillImageLayers(vector<vector<Color> >& layers, size_t* pWidth, size_t* pHeight, size_t* pDepth)
{
	uint w;
	uint h;
	uint d;
	vector<const Color*> pData;
	getVoxelsAtCorrectScale(&pData, &w, &h, &d);

	layers.resize(d);
	for(uint z=0; z<d; z++)
	{
		vector<Color>& layer = layers[z];
		layer.resize(w*h);

		for(uint y=0; y<h; y++)
		{
			for(int x=0; x<w; x++)
			{
				const size_t s = (size_t)x+((size_t)y*(size_t)w)+((size_t)z*(size_t)h*(size_t)w);
				const size_t t = x+y*w;
				assert(s < pData.size());
				if(pData[s] != nullptr)
				{
					assert(t < layer.size());
					layer[t] = *pData[s];
				}
			}
		}
	}

	if(pWidth != nullptr)
	{
		*pWidth = w;
	}
	if(pHeight != nullptr)
	{
		*pHeight = h;
	}
	if(pDepth != nullptr)
	{
		*pDepth = d;
	}
}

error Scene::saveAsPngArray(const string& targetFolderPath)
{
	size_t width = 0;
	size_t height = 0;
	size_t depth = 0;

	vector<vector<Color>> imageLayers;
	fillImageLayers(imageLayers, &width, &height, &depth);

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
		error err = savePng(layer, width, height, outputPath, true);
		if(err != ""s)
		{
			return err;
		}
	}

	return ""s;
}

error Scene::saveAsMergedPng(const string& targetFilePath)
{
	size_t width = 0;
	size_t height = 0;
	size_t depth = 0;

	vector<vector<Color>> imageLayers;
	fillImageLayers(imageLayers, &width, &height, &depth);

	string filePath = expandTargetFilePath(targetFilePath, width, height, depth);

	bool drawBorder=1;

	size_t imgWidth = (width * depth)+(drawBorder ? (depth-1) : 0);
	size_t imgHeight = height;

	vector<Color> data;
	data.resize(imgWidth*imgHeight*depth);

	for(size_t z=0; z<imageLayers.size(); z++)
	{
		const vector<Color>& image2d = imageLayers[z];
		for(size_t y=0; y<height; y++)
		{
			size_t yPos = y;
			for(size_t x=0; x<width; x++)
			{
				size_t xPos = (z*width+x);

				size_t targetIndex = (xPos + imgWidth*yPos)+(drawBorder ? z : 0);
				size_t sourceIndex = x + width*y;
				assert(targetIndex < data.size());
				assert(sourceIndex < image2d.size());
				data[targetIndex] = image2d[sourceIndex];
			}
			if(drawBorder && z > 0)
			{
				size_t xPos = z*width;
				size_t targetIndex = (((xPos + imgWidth*yPos)+z)-1);
				data[targetIndex].r=0;
				data[targetIndex].g=0;
				data[targetIndex].b=0;
				data[targetIndex].a=255;
			}
		}
	}

	if(width == 0 || height == 0)
	{
		return "cannot write an image of width or height 0."s;
	}

	return savePng(data, imgWidth, imgHeight, filePath, true);
}
