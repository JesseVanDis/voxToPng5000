
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
		error 			saveAsMergedPng(const string& targetFilePath);

		const Color* 	getVoxel(int x, int y, int z) const;
		const Color& 	lookupPaletteColor(uint8_t colorIndex) const;
		void  			getBounds(int* pX0, int* pY0, int* pZ0, int* pX1, int* pY1, int* pZ1);

	protected:
		void 			registerNodeChild(Node* pParent, NodeChild* pChild);
		void 			registerShape(NodeShapeModel* pShape);

	private:
		void 			fillImageLayers(vector<vector<Color> >& layers, size_t* pWidth, size_t* pHeight, size_t* pDepth);
		string 			expandTargetFilePath(const string& targetFilePath, size_t sceneSizeX, size_t sceneSizeY, size_t sceneSizeZ) const;

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
