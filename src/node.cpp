
#include <math.h>
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
	return "";
}

int32_t Node::getId() const
{
	return m_id;
}

void Node::getGlobalTransformation(int* pBuffer) const
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
			MATRIXARRAY_MUL_TO(nodeTransformation, current, multiplied);
			memcpy(current, multiplied, sizeof(current));
		}
	}
	memcpy(pBuffer, current, sizeof(current));
}

void Node::transformGlobal(int x, int y, int z, int* pX, int* pY, int* pZ, bool snapToVoxel) const
{
	int globalTransformation[16];
	getGlobalTransformation(globalTransformation);

	float pos[4] = {(float)x, (float)y, (float)z, 1};
	float result[4] = {0, 0, 0, 0};

	if(snapToVoxel)
	{
		pos[0] += 0.5f;
		pos[1] += 0.5f;
		pos[2] += 0.5f;
	}

	float globalTransformationF[16];
	for(size_t i=0; i<16; i++)
	{
		globalTransformationF[i] = (float)globalTransformation[i];
	}

	MATRIX_MUL_TO(globalTransformation, 4, 4, pos, 4, 1, result);

	if(pX != nullptr)
	{
		*pX = (int)floor(result[0]);
	}
	if(pY != nullptr)
	{
		*pY = (int)floor(result[1]);
	}
	if(pZ != nullptr)
	{
		*pZ = (int)floor(result[2]);
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
	return "";
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


