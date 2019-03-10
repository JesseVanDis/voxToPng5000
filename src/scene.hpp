
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

class Scene
{
	friend class NodeChild;
	friend class NodeShapeModel;

	public:
		Scene();
		error 			load(const string& voxFilePath);
		error 			saveAsPngArray(const string& targetFolderPath);
		error 			saveAsMergedPng(const string& targetFilePath, const Color* pBorderColor = nullptr);

		const Color* 	getVoxel(int x, int y, int z) const; // note that everything has ben rescaled by 2. so to get voxel x=3 you may want to input 6. too lazy to fix this right now
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
		void 			fillImageLayers(vector<vector<Color> >& layers, size_t* pWidth, size_t* pHeight, size_t* pDepth, int* pScenePosX, int* pScenePosY, int* pScenePosZ);
		string 			expandTargetFilePath(const string& targetFilePath, size_t sceneSizeX, size_t sceneSizeY, size_t sceneSizeZ, int scenePosX, int scenePosY, int scenePosZ) const;

		struct ChildToLink
		{
			Node* pParent;
			NodeChild* pChild;
		};

		vector<char> 				m_data;
		weak_ptr<Node>				m_pRootNode;
		vector<shared_ptr<Model>>	m_models;
		vector<shared_ptr<Node>> 	m_nodes;
		Palette						m_palette;

		vector<ChildToLink> 		m_allNodeChildren;
		vector<NodeShapeModel*> 	m_allShapes;
};


#endif //VOXTOPNG_SCENE_HPP
