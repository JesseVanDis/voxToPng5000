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

struct ExpandFilepathContext
{
	string targetFilePath;
	struct
	{
		int posX = 0;
		int posY = 0;
		int posZ = 0;
		uint width = 0;
		uint height = 0;
		uint depth = 0;
	} scene;

	struct
	{
		int posX = 0;
		int posY = 0;
		int posZ = 0;
		uint width = 0;
		uint height = 0;
		uint depth = 0;
	} chunk;
	uint numChunks = 0;
};

Scene::Scene(bool verboseEnabled)
{
	m_verboseEnabled = verboseEnabled;
}

error Scene::load(const string& voxFilePath)
{
	error err;

	if(m_verboseEnabled)
	{
		printf("loading: %s\n", voxFilePath.c_str());
	}

	Context context;
	context.pScene = this;

	// load data
	{
		FILE *voxHandle = fopen(voxFilePath.c_str(), "rb");
		if (!voxHandle)
		{
			return "Couldn't open .vox file, are you sure it exists?";
		}
		fseek(voxHandle, 0, SEEK_END);
		auto voxLength = ftell(voxHandle);
		fseek(voxHandle, 0, SEEK_SET);
		m_data.resize((size_t)(voxLength + 1));
		size_t bytes = fread(m_data.data(), (size_t)voxLength, 1, voxHandle);
		if(bytes == 0)
		{
			return "failed to read.vox file";
		}
		fclose(voxHandle);
	}
	Loader loader(m_data);

	// models
	int32_t nextModelId = 0;
	bool modelFound;
	do
	{
		modelFound = false;
		string key = loader.seek("SIZE", true);
		if(!key.empty())
		{
			shared_ptr<Model> pModel(new Model(nextModelId++));
			err = pModel->loadSize(loader);
			if(!err.empty())
			{
				return err;
			}

			key = loader.seek("XYZI", true);
			if(!key.empty())
			{
				if(m_verboseEnabled)
				{
					printf("loading model '%d'\n", nextModelId);
				}

				err = pModel->loadData(loader);
				if(!err.empty())
				{
					return err;
				}
				modelFound = true;
				m_models.push_back(pModel);
			}
			else
			{
				return "'SIZE' token found but was not followed by a 'XYZI' tag";
			}
		}
	} while(modelFound);

	// palette
	loader.reset();
	{
		if(m_verboseEnabled)
		{
			printf("loading palette\n");
		}
		string key = loader.seek("RGBA", true);
		if(!key.empty())
		{
			err = m_palette.load(loader);
			if(!err.empty())
			{
				return err;
			}
		}
	}

	// nodes
	loader.reset();
	if(m_verboseEnabled)
	{
		printf("loading nodes\n");
	}
	bool nodeFound;
	do
	{
		nodeFound = false;
		const static vector<string> s_keys = {"nTRN", "nGRP", "nSHP"};
		string key = loader.seekAny(s_keys, true);
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
			if(!err.empty())
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
			assert(link.pChild != nullptr);
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

	if(m_verboseEnabled)
	{
		printf("handling transformations of models/nodes\n");
	}
	doubleScale();
	recenterOrigins();

#ifdef PRINT_VOXELS
	printVoxels(); // for debugging
#endif

	return "";
}

void Scene::registerNodeChild(Node* pParent, NodeChild* pChild)
{
	assert(pParent != nullptr);
	assert(pChild != nullptr);
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
		return "Failed to write Png image";
	}
	return "";
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

const Color& Scene::lookupPaletteColor(uint8_t colorIndex) const
{
	return m_palette.lookupColor(colorIndex);
}

string Scene::expandTargetFilePath(const ExpandFilepathContext& context) const
{
	string result = context.targetFilePath;
	if(context.numChunks > 1)
	{
		if(((result.find("{CHUNKPOS_X}", 0)) == std::string::npos) && ((result.find("{CHUNKPOS_Y}", 0)) == std::string::npos) && ((result.find("{CHUNKPOS_Z}", 0)) == std::string::npos))
		{
			replaceAll(result, ".png", "_{CHUNKPOS_X}_{CHUNKPOS_Y}_{CHUNKPOS_Z}");
		}
	}
	replaceAll(result, "{SIZE_X}", to_string(context.scene.width));
	replaceAll(result, "{SIZE_Y}", to_string(context.scene.height));
	replaceAll(result, "{SIZE_Z}", to_string(context.scene.depth));
	replaceAll(result, "{POS_X}", to_string(context.scene.posX));
	replaceAll(result, "{POS_Y}", to_string(context.scene.posY));
	replaceAll(result, "{POS_Z}", to_string(context.scene.posZ));
	replaceAll(result, "{CHUNKSIZE_X}", to_string(context.chunk.width));
	replaceAll(result, "{CHUNKSIZE_Y}", to_string(context.chunk.height));
	replaceAll(result, "{CHUNKSIZE_Z}", to_string(context.chunk.depth));
	replaceAll(result, "{CHUNKPOS_X}", to_string(context.chunk.posX));
	replaceAll(result, "{CHUNKPOS_Y}", to_string(context.chunk.posY));
	replaceAll(result, "{CHUNKPOS_Z}", to_string(context.chunk.posZ));
	return result;
}

void Scene::doubleScale()
{
	// multiply positions
	for(auto&& node : m_nodes)
	{
		if(const NodeTransform* pTransformConst = node->toNodeTransform())
		{
			NodeTransform* pTransform = const_cast<NodeTransform*>(pTransformConst); // yeah ugly I know. wanna fight ? // NOLINT
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
				NodeTransform* pTransform = const_cast<NodeTransform*>(pParentTransformConst); // yeah ugly I know. wanna fight ? // NOLINT
				int32_t w = 0;
				int32_t h = 0;
				int32_t d = 0;
				pShape->getSize(&w, &h, &d);

				const float* globalTransformationF = pTransform->getGlobalTransformation();
				int globalTransformation[16];
				for(size_t i=0; i<16; i++)
				{
					globalTransformation[i] = (int)globalTransformationF[i];
				}

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

					const float* globalTransformationF = pShape->getGlobalTransformation();
					int globalTransformation[16];
					for(size_t i=0; i<16; i++)
					{
						globalTransformation[i] = (int)globalTransformationF[i];
					}

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


void Scene::getVoxelsAtCorrectScale(vector<const Color*>* pVoxels, uint* pSceneWidth, uint* pSceneHeight, uint* pSceneDepth, int* pScenePosX, int* pScenePosY, int* pScenePosZ) const
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

	uint numNodeShapes = 0u;
	for(auto&& node : m_nodes)
	{
		if (node->toNodeShape() != nullptr)
		{
			numNodeShapes++;
		}
	}

	uint shapeNodeIndex = 0;
	if(pVoxels != nullptr)
	{
		pVoxels->resize((((size_t)w)*((size_t)h)*((size_t)d)), nullptr);

		for(const auto& node : m_nodes)
		{
			if(const NodeShape* pShape = node->toNodeShape())
			{
				if(m_verboseEnabled)
				{
					string name = node->getName(true);
					if(name.empty())
					{
						printf("(%u/%u) loading voxels of nodeId: %d\n", shapeNodeIndex, numNodeShapes, node->getId());
					}
					else
					{
						printf("(%u/%u) loading voxels of nodeId: %d (%s)\n", shapeNodeIndex, numNodeShapes, node->getId(), name.c_str());
					}
				}
				int x0l = INT32_MAX;
				int y0l = INT32_MAX;
				int z0l = INT32_MAX;
				int x1l = -INT32_MAX;
				int y1l = -INT32_MAX;
				int z1l = -INT32_MAX;
				pShape->getGlobalBounds(&x0l, &y0l, &z0l, &x1l, &y1l, &z1l);

				for(auto&& model : m_models)
				{
					int32_t modelId = model->getId();
					if(pShape->hasModelId(model->getId()))
					{
						int z=z0l;
						assert(z0l-z0 >= 0);
						size_t zi=(size_t)(z0l-z0)/2;
						for(; z<z1l; z+=2)
						{
							int y=y0l;
							assert(y0l-y0 >= 0);
							size_t yi= (size_t)(y0l - y0)/2;
							for(; y<y1l; y+=2)
							{
								int x=x0l;
								assert(x0l-x0 >= 0);
								size_t xi=(size_t)(x0l-x0)/2;
								for(; x<x1l; x+=2)
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
				shapeNodeIndex++;
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
	if(pScenePosX != nullptr)
	{
		*pScenePosX = x0/2;
	}
	if(pScenePosY != nullptr)
	{
		*pScenePosY = y0/2;
	}
	if(pScenePosZ != nullptr)
	{
		*pScenePosZ = z0/2;
	}
}

static size_t removeHiddenVoxelsFromData(vector<const Color*>* pData, size_t w, size_t h, size_t d)
{
	assert(pData != nullptr);
	const vector<const Color*> original = *pData;
	vector<const Color*>& data = *pData;
	size_t numVoxelsRemoved = 0u;

	size_t surroundingIndices[9*3];

	for(size_t z1=1; z1<(d-1); z1++)
	{
		size_t z0 = z1-1u;
		size_t z2 = z1+1u;
		for(size_t y1=1; y1<(h-1); y1++)
		{
			size_t y0 = y1-1u;
			size_t y2 = y1+1u;
			for(size_t x1=1; x1<(w-1); x1++)
			{
				size_t x0 = x1-1u;
				size_t x2 = x1+1u;

				surroundingIndices[13] = x1 + ( y1 * w ) + ( z1 * h * w );  // self

				if(original[surroundingIndices[13]] == nullptr)
				{
					continue;
				}

				// back
				surroundingIndices[0]  = x0 + ( y0 * w ) + ( z0 * h * w );
				surroundingIndices[1]  = x1 + ( y0 * w ) + ( z0 * h * w );
				surroundingIndices[2]  = x2 + ( y0 * w ) + ( z0 * h * w );
				surroundingIndices[3]  = x0 + ( y1 * w ) + ( z0 * h * w );
				surroundingIndices[4]  = x1 + ( y1 * w ) + ( z0 * h * w );
				surroundingIndices[5]  = x2 + ( y1 * w ) + ( z0 * h * w );
				surroundingIndices[6]  = x0 + ( y2 * w ) + ( z0 * h * w );
				surroundingIndices[7]  = x1 + ( y2 * w ) + ( z0 * h * w );
				surroundingIndices[8]  = x2 + ( y2 * w ) + ( z0 * h * w );

				// middle
				surroundingIndices[9]  = x0 + ( y0 * w ) + ( z1 * h * w );
				surroundingIndices[10] = x1 + ( y0 * w ) + ( z1 * h * w );
				surroundingIndices[11] = x2 + ( y0 * w ) + ( z1 * h * w );
				surroundingIndices[12] = x0 + ( y1 * w ) + ( z1 * h * w );
				// surroundingIndices[13] .................  self
				surroundingIndices[14] = x2 + ( y1 * w ) + ( z1 * h * w );
				surroundingIndices[15] = x0 + ( y2 * w ) + ( z1 * h * w );
				surroundingIndices[16] = x1 + ( y2 * w ) + ( z1 * h * w );
				surroundingIndices[17] = x2 + ( y2 * w ) + ( z1 * h * w );

				// front
				surroundingIndices[18] = x0 + ( y0 * w ) + ( z2 * h * w );
				surroundingIndices[19] = x1 + ( y0 * w ) + ( z2 * h * w );
				surroundingIndices[20] = x2 + ( y0 * w ) + ( z2 * h * w );
				surroundingIndices[21] = x0 + ( y1 * w ) + ( z2 * h * w );
				surroundingIndices[22] = x1 + ( y1 * w ) + ( z2 * h * w );
				surroundingIndices[23] = x2 + ( y1 * w ) + ( z2 * h * w );
				surroundingIndices[24] = x0 + ( y2 * w ) + ( z2 * h * w );
				surroundingIndices[25] = x1 + ( y2 * w ) + ( z2 * h * w );
				surroundingIndices[26] = x2 + ( y2 * w ) + ( z2 * h * w );

				bool keep = false;
				for(size_t i=0; i<27; i++) // NOLINT
				{
					assert(surroundingIndices[i] < original.size());
					if(original[surroundingIndices[i]] == nullptr)
					{
						keep = true;
						break;
					}
				}

				if(!keep)
				{
					data[surroundingIndices[13]] = nullptr;
					numVoxelsRemoved++;
				}
			}
		}
	}

	return numVoxelsRemoved;
}

static void clipVoxelsFromEdges(vector<const Color*>* pData, uint w, uint h, uint d, int left, int back, int bottom, int right, int front, int top)
{
	assert(pData != nullptr);
	vector<const Color*>& data = *pData;
	const int clipping[6] = {left, back, bottom, right, front, top};
	for(size_t z=0; z<d; z++)
	{
		for(size_t y=0; y<h; y++)
		{
			for(size_t x=0; x<w; x++)
			{
				bool keep = x >= clipping[0]
						 && y >= clipping[1]
						 && z >= clipping[2]
						 && x < (w-clipping[3])
						 && y < (h-clipping[4])
						 && z < (d-clipping[5]);

				if(!keep)
				{
					size_t t = x + ( y * w ) + ( z * h * w );
					data[t] = nullptr;
				}
			}
		}
	}
}

static size_t getNumVoxels(const vector<const Color*>& data)
{
	size_t numVoxels = 0u;
	for(auto&& v : data)
	{
		if(v != nullptr)
		{
			numVoxels++;
		}
	}
	return numVoxels;
}

void Scene::fillImageLayers(vector<ImageLayer>& layers, uint* pWidth, uint* pHeight, uint* pDepth, int* pScenePosX, int* pScenePosY, int* pScenePosZ, const SavingContext& context)
{
	uint w;
	uint h;
	uint d;
	vector<const Color*> data;
	getVoxelsAtCorrectScale(&data, &w, &h, &d, pScenePosX, pScenePosY, pScenePosZ);

	size_t numVoxelsBeforeCleanup = getNumVoxels(data);

	if(context.removeHiddenVoxels)
	{
		if(m_verboseEnabled)
		{
			printf("num voxels currently: %u\n", (uint)getNumVoxels(data));
			printf("removing hidden voxels...\n");
		}
		removeHiddenVoxelsFromData(&data, w, h, d);
	}

	if((context.clipping.left != 0) || (context.clipping.back != 0) || (context.clipping.bottom != 0) || (context.clipping.right != 0) || (context.clipping.front != 0) || (context.clipping.top != 0))
	{
		if(m_verboseEnabled)
		{
			printf("clipping edges...\n");
		}
		clipVoxelsFromEdges(&data, w, h, d, context.clipping.left, context.clipping.back, context.clipping.bottom, context.clipping.right, context.clipping.front, context.clipping.top);
	}

	if(m_verboseEnabled)
	{
		size_t numVoxels = getNumVoxels(data);
		if(numVoxelsBeforeCleanup != numVoxels)
		{
			printf("num voxels reduced by: %u%%\n", (uint)((1.0 - ((double)numVoxels / (double)numVoxelsBeforeCleanup)) * 100.0) );
		}
		printf("total voxel count: %u\n", (uint)getNumVoxels(data));
	}

	layers.resize(d);
	for(uint z=0; z<d; z++)
	{
		vector<Color>& layer = layers[z].pixels;
		layer.resize(w*h);

		for(uint y=0; y<h; y++)
		{
			for(int x=0; x<w; x++)
			{
				const size_t s = (size_t)x+((size_t)y*(size_t)w)+((size_t)z*(size_t)h*(size_t)w);
				const size_t t = x+y*w;
				assert(s < data.size());
				if(data[s] != nullptr)
				{
					assert(t < layer.size());
					layer[t] = *data[s];
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

bool Chunk::isPointInside(int x, int y, int z) const
{
	return x >= posX && y >= posY && z >= posZ && x < (int)(posX + width) && y < (int)(posY + height) && z < (int)(posZ + depth);
}


static vector<Chunk> splitImageLayersToChunks(const vector<ImageLayer>& layers, int centerPosX, int centerPosY, int centerPosZ, uint sceneWidth, uint sceneHeight, uint sceneDepth, uint chunkWidth, uint chunkHeight, uint chunkDepth, int offsetX, int offsetY, int offsetZ)
{
	vector<Chunk> chunks;

	int startX = 0;
	int startY = 0;
	int startZ = 0;
	int endX = 0;
	int endY = 0;
	int endZ = 0;

	if(chunkWidth > 0)
	{
		startX = offsetX;
		while(startX > -centerPosX)
		{
			startX -= chunkWidth;
		}

		endX = startX;
		while(endX <= (-centerPosX+(int)sceneWidth))
		{
			endX += chunkWidth;
		}
	}
	else
	{
		startX = -centerPosX;
		endX = startX+(int)sceneWidth;
		chunkWidth=(int)sceneWidth;
	}

	if(chunkHeight > 0)
	{
		startY = offsetY;
		while(startY > -centerPosY)
		{
			startY -= chunkHeight;
		}

		endY = startY;
		while(endY <= (-centerPosY+(int)sceneHeight))
		{
			endY += chunkHeight;
		}
	}
	else
	{
		startY = -centerPosY;
		endY = startY+sceneHeight;
		chunkHeight=sceneHeight;
	}

	if(chunkDepth > 0)
	{
		startZ = offsetZ;
		while(startZ > -centerPosZ)
		{
			startZ -= chunkDepth;
		}

		endZ = startZ;
		while(endZ <= (-centerPosZ+(int)sceneDepth))
		{
			endZ += chunkDepth;
		}
	}
	else
	{
		startZ = -centerPosZ;
		endZ = startZ+sceneDepth;
		chunkDepth=sceneDepth;
	}


	for(int x=startX; x<endX; x+=chunkWidth) for(int y=startY; y<endY; y+=chunkHeight) for(int z=startZ; z<endZ; z+=chunkDepth)
	{
		Chunk chunk;
		chunk.posX = x;
		chunk.posY = y;
		chunk.posZ = z;
		chunk.width = chunkWidth;
		chunk.height = chunkHeight;
		chunk.depth = chunkDepth;
		chunk.layers.resize(chunkDepth);
		for(auto&& layer : chunk.layers)
		{
			layer.pixels.resize(chunkWidth * chunkHeight);
		}
		chunks.push_back(chunk);
	}

	for(size_t imgZ=0; imgZ < layers.size(); imgZ++)
	{
		const int z = (int)imgZ - centerPosZ;
		const ImageLayer& layer = layers[imgZ];
		for(size_t imgY=0; imgY<sceneHeight; imgY++)
		{
			const int y = (int)imgY - centerPosY;
			for(size_t imgX=0; imgX<sceneWidth; imgX++)
			{
				const int x = (int)imgX - centerPosX;
				const size_t sourceIndex = imgX + (imgY * sceneWidth);
				Chunk* pChunk = nullptr;
				for(auto&& chunk : chunks)
				{
					if(chunk.isPointInside(x,y,z))
					{
						pChunk = &chunk;
						break;
					}
				}
				assert(pChunk != nullptr);
				const int chunkVoxelPosX = x - pChunk->posX;
				const int chunkVoxelPosY = y - pChunk->posY;
				const int chunkVoxelPosZ = z - pChunk->posZ;
				assert(chunkVoxelPosX >= 0);
				assert(chunkVoxelPosY >= 0);
				assert(chunkVoxelPosZ >= 0);

				assert(chunkVoxelPosZ < pChunk->layers.size());
				ImageLayer& chunkImageLayer = pChunk->layers[chunkVoxelPosZ];
				size_t targetIndex = chunkVoxelPosX + ((size_t)chunkVoxelPosY * pChunk->width);
				assert(targetIndex < chunkImageLayer.pixels.size());
				assert(sourceIndex < layer.pixels.size());
				chunkImageLayer.pixels[targetIndex] = layer.pixels[sourceIndex];
			}
		}
	}
	return chunks;
}

error Scene::saveAsPngArray(const string& targetFolderPath, const SavingContext& context)
{
	if(m_verboseEnabled)
	{
		printf("saving as multiple png's to: %s\n", targetFolderPath.c_str());
	}

	uint width = 0;
	uint height = 0;
	uint depth = 0;

	vector<ImageLayer> imageLayers;
	fillImageLayers(imageLayers, &width, &height, &depth, nullptr, nullptr, nullptr, context);

	for(size_t i=0; i<imageLayers.size(); i++)
	{
		vector<Color>& layer = imageLayers[i].pixels;
		string imageName = "output_";
		imageName += to_string(i);
		imageName += ".png";

		string outputPath(targetFolderPath);
		outputPath += "/";
		outputPath += imageName;

		if(width == 0 || height == 0)
		{
			return "cannot write an image of width or height 0.";
		}
		error err = savePng(layer, width, height, outputPath, true);
		if(!err.empty())
		{
			return err;
		}
	}

	return "";
}

error Scene::saveAsMergedPng(const string& targetFilePath, const SavingContext& context)
{
	uint width = 0;
	uint height = 0;
	uint depth = 0;
	int posX = 0;
	int posY = 0;
	int posZ = 0;

	vector<ImageLayer> globalImageLayers;
	fillImageLayers(globalImageLayers, &width, &height, &depth, &posX, &posY, &posZ, context);

	vector<Chunk> chunks = splitImageLayersToChunks(globalImageLayers, -posX, -posY, -posZ, width, height, depth, context.chunkSetup.sizeX, context.chunkSetup.sizeY, context.chunkSetup.sizeZ, context.chunkSetup.offsetX, context.chunkSetup.offsetY, context.chunkSetup.offsetZ);

	for(auto&& chunk : chunks)
	{
		const vector<ImageLayer>& imageLayers = chunk.layers;

		ExpandFilepathContext expandFilepathContext;
		expandFilepathContext.targetFilePath = targetFilePath;
		expandFilepathContext.scene.posX = posX;
		expandFilepathContext.scene.posY = posY;
		expandFilepathContext.scene.posZ = posZ;
		expandFilepathContext.scene.width = width;
		expandFilepathContext.scene.height = height;
		expandFilepathContext.scene.depth = depth;
		expandFilepathContext.numChunks = (uint)chunks.size();
		expandFilepathContext.chunk.posX = chunk.posX;
		expandFilepathContext.chunk.posY = chunk.posY;
		expandFilepathContext.chunk.posZ = chunk.posZ;
		expandFilepathContext.chunk.width = chunk.width;
		expandFilepathContext.chunk.height = chunk.height;
		expandFilepathContext.chunk.depth = chunk.depth;

		const string filePath = expandTargetFilePath(expandFilepathContext);
		const bool drawBorder = context.pBorderColor != nullptr;
		const size_t imgWidth = (chunk.width * chunk.depth)+(drawBorder ? (chunk.depth-1) : 0);
		const size_t imgHeight = chunk.height;

		const size_t numPixels = imgWidth*imgHeight;

		if(m_verboseEnabled)
		{
			printf("saving as single png to: %s. (canvas size: %u, %u)\n", filePath.c_str(), (uint)imgWidth, (uint)imgHeight);
		}

		vector<Color> data;
		if(numPixels >= data.max_size())
		{
			char error[1024];
			sprintf(error, "cannot create an image of size: %u, %u. its too big. max amount of pixels is: %u\n", (uint)imgWidth, (uint)imgHeight, (uint)(data.max_size()));
			return string(error);
		}

		data.resize(numPixels);

		for(size_t z=0; z<imageLayers.size(); z++)
		{
			const vector<Color>& image2d = imageLayers[z].pixels;
			for(size_t y=0; y<chunk.height; y++)
			{
				size_t yPos = y;
				for(size_t x=0; x<chunk.width; x++)
				{
					size_t xPos = (z*chunk.width+x);

					size_t targetIndex = (xPos + imgWidth*yPos)+(drawBorder ? z : 0);
					size_t sourceIndex = x + chunk.width*y;
					assert(targetIndex < data.size());
					assert(sourceIndex < image2d.size());
					data[targetIndex] = image2d[sourceIndex];
				}
				if(drawBorder && z > 0)
				{
					size_t xPos = z*chunk.width;
					size_t targetIndex = (((xPos + imgWidth*yPos)+z)-1);
					data[targetIndex] = *context.pBorderColor;
				}
			}
		}

		if(chunk.width == 0 || chunk.height == 0)
		{
			return "cannot write an image of width or height 0.";
		}

		error err = savePng(data, imgWidth, imgHeight, filePath, true);
		if(!err.empty())
		{
			return err;
		}
	}
	return "";
}
