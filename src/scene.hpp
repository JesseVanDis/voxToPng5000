
#ifndef VOXTOPNG_SCENE_HPP
#define VOXTOPNG_SCENE_HPP

#include <memory>
#include <string>
#include <vector>
#include "common.hpp"
#include "palette.hpp"

using namespace std;

class Node;
class Model;
class NodeChild;
class NodeShapeModel;

struct SavingContext
{
	const Color* 	pBorderColor = nullptr;
	bool 			removeHiddenVoxels = false;

	struct
	{
		int left = 0;
		int back = 0;
		int bottom = 0;
		int right = 0;
		int front = 0;
		int top = 0;
	} clipping;

	struct
	{
		uint sizeX = 0;
		uint sizeY = 0;
		uint sizeZ = 0;
		int offsetX = 0;
		int offsetY = 0;
		int offsetZ = 0;
	} chunkSetup;
};

struct ImageLayer
{
	vector<Color> pixels;
};

struct Chunk
{
	vector<ImageLayer> layers;
	int posX;
	int posY;
	int posZ;
	uint sizeX;
	uint sizeY;
	uint sizeZ;

	bool isPointInside(int x, int y, int z) const;
};

class Scene
{
	friend class NodeChild;
	friend class NodeShapeModel;

	public:
		explicit Scene(bool verboseEnabled = false);
		error 			load(const string& voxFilePath);
		error 			saveAsPngArray(const string& targetFolderPath, const SavingContext& context = SavingContext());
		error 			saveAsMergedPng(const string& targetFilePath, const SavingContext& context = SavingContext());

		const Color& 	lookupPaletteColor(uint8_t colorIndex) const;
		void  			getBounds(int* pX0, int* pY0, int* pZ0, int* pX1, int* pY1, int* pZ1) const;

		void 			getVoxelsAtCorrectScale(vector<const Color*>* pVoxels, uint* pSceneWidth, uint* pSceneHeight, uint* pSceneDepth, int* pScenePosX, int* pScenePosY, int* pScenePosZ) const;

	protected:
		void 			registerNodeChild(Node* pParent, NodeChild* pChild);
		void 			registerShape(NodeShapeModel* pShape);

	private:
		void 			doubleScale(); // to fix pivotpoints that are not really at the center ( for example a 3,3,3 cube should have a pivot of 1.5, 1.5, 1.5 . but it has 1,1,1 and it messes with the rotation system )
		void 			recenterOrigins(); // "part of the 'doubleScale()' pivotpoint hackfix"
		void 			printVoxels();
		void 			fillImageLayers(vector<ImageLayer>& layers, uint* pWidth, uint* pHeight, uint* pDepth, int* pScenePosX, int* pScenePosY, int* pScenePosZ, const SavingContext& context = SavingContext());
		string 			expandTargetFilePath(const string& targetFilePath, uint sceneSizeX, uint sceneSizeY, uint sceneSizeZ, int scenePosX, int scenePosY, int scenePosZ, uint numChunks, int chunkPosX, int chunkPosY, int chunkPosZ) const;

		struct ChildToLink
		{
			Node* pParent = nullptr;
			NodeChild* pChild = nullptr;
		};

		vector<char> 				m_data;
		weak_ptr<Node>				m_pRootNode;
		vector<shared_ptr<Model>>	m_models;
		vector<shared_ptr<Node>> 	m_nodes;
		Palette						m_palette;

		vector<ChildToLink> 		m_allNodeChildren;
		vector<NodeShapeModel*> 	m_allShapes;

		bool 						m_verboseEnabled;
};


#endif //VOXTOPNG_SCENE_HPP
