
#include <cstring>
#include <assert.h>
#include "node.hpp"
#include "loader.hpp"
#include "scene.hpp"
#include "node_group.hpp"
#include "node_shape.hpp"
#include "node_transform.hpp"

Node::Node(const Context& context)
	: m_context(context)
{
}

error Node::load(Loader& loader)
{
	__attribute__((unused)) int32_t chunkSize = loader.readNextInt32();
	__attribute__((unused)) int32_t numChildrenChunks = loader.readNextInt32();

	m_id = loader.readNextInt32();
	m_dictionary.load(loader);
	return ""s;
}

int32_t Node::getId() const
{
	return m_id;
}

void Node::getGlobalPosition(int* pX, int* pY, int* pZ) const
{
	transformGlobal(0, 0, 0, pX, pY, pZ);

	/*
	int x = 0;
	int y = 0;
	int z = 0;
	const Node* pParent = this;

	if(const NodeShape* pShape = this->toNodeShape())
	{
		int w = 0;
		int h = 0;
		int d = 0;
		pShape->getSize(&w, &h, &d);
		x -= w / 2;
		y -= h / 2;
		z -= d / 2;
	}

	while(pParent != nullptr)
	{
		if(const NodeTransform* pTransform = pParent->toNodeTransform())
		{
			int tx = 0;
			int ty = 0;
			int tz = 0;
			pTransform->getTranslation(&tx, &ty, &tz);
			x += tx;
			y += ty;
			z += tz;
		}
		pParent = pParent->getParent();
	}
	if(pX != nullptr)
	{
		*pX = x;
	}
	if(pY != nullptr)
	{
		*pY = y;
	}
	if(pZ != nullptr)
	{
		*pZ = z;
	}
	*/
}

void Node::transformGlobal(int x, int y, int z, int* pX, int* pY, int* pZ) const
{
	int current[16];
	memcpy(current, s_identity, sizeof(current));

	vector<const Node*> hierarchy = getHierarchy();

	for(size_t i=0; i<hierarchy.size(); i++)
	{
		if(const NodeTransform* pTransform = hierarchy[i]->toNodeTransform())
		{
			int nodeTransformation[16];
			int multiplied[16];
			pTransform->getTransformation(nodeTransformation);

			// nodepos: -4, 0, 3

			// it should be rotating at exacly the center!!

			// rotate then translation following the rotation ??


			MATRIXARRAY_MUL_TO(nodeTransformation, current, multiplied);
			//MATRIXARRAY_MUL_TO(current, nodeTransformation, multiplied);

			memcpy(current, multiplied, sizeof(current));
		}
	}

	int pos[4] = {x, y, z, 1};
	int result[4] = {0, 0, 0, 0};

	MATRIX_MUL_TO(current, 4, 4, pos, 4, 1, result);
	//MATRIX_MUL_TO(pos, 1, 4, current, 4, 4, result);

	if(pX != nullptr)
	{
		*pX = result[0];
	}
	if(pY != nullptr)
	{
		*pY = result[1];
	}
	if(pZ != nullptr)
	{
		*pZ = result[2];
	}
}

vector<const Node*> Node::getHierarchy() const
{
	vector<const Node*> hierarchyReverserd;
	hierarchyReverserd.reserve(32);
	const Node* pParent = this;
	while(pParent != nullptr)
	{
		hierarchyReverserd.push_back(pParent);
		pParent = pParent->getParent();
	}
	vector<const Node*> hierarchy;
	hierarchy.reserve(hierarchyReverserd.size());
	for(int i=(int)hierarchyReverserd.size()-1; i>=0; i--)
	{
		hierarchy.push_back(hierarchyReverserd[i]);
	}
	return hierarchy;
}

void Node::setParent(const Node* pParent)
{
	m_pParent = pParent;
}

const Node* Node::getParent() const
{
	return m_pParent;
}

const Context& Node::getContext() const
{
	return m_context;
}

NodeChild::NodeChild(Node* pParent, const Context& context)
	: m_context(context)
{
	m_context.pScene->registerNodeChild(pParent, this);
}

error NodeChild::load(Loader& loader)
{
	m_id = loader.readNextInt32();
	return ""s;
}

int32_t NodeChild::getId() const
{
	return m_id;
}

const weak_ptr<Node>& NodeChild::getNode() const
{
	return m_pNode;
}

void NodeChild::setNodePtr(const weak_ptr<Node>& pNode)
{
	m_pNode = pNode;
}


