
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
	return ""s;
}

int32_t Node::getId() const
{
	return m_id;
}

void Node::getGlobalPosition(int* pX, int* pY, int* pZ) const
{
	transformGlobal(0,0,0,pX,pY,pZ, false);
	/*
	int globalTransformation[16];

	getGlobalTransformation(globalTransformation);

	int pos[4] = {0, 0, 0, 1};
	int result[4] = {0, 0, 0, 0};

	MATRIX_MUL_TO(globalTransformation, 4, 4, pos, 4, 1, result);
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
	 */
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

void Node::getLocalTransformation(int* pBuffer) const
{
	const Node* pParent = this;
	while(pParent != nullptr)
	{
		if(const NodeTransform* pTransform = pParent->toNodeTransform())
		{
			pTransform->getTransformation(pBuffer);
			return;
		}
		pParent = pParent->getParent();
	}
	memcpy(pBuffer, s_identity, sizeof(s_identity));
}

/*
void Node::transformGlobal(int x, int y, int z, int* pX, int* pY, int* pZ) const
{
	int globalTransformation[16];

	getGlobalTransformation(globalTransformation);

	int pos[4] = {x, y, z, 1};
	int result[4] = {0, 0, 0, 0};

	int translationOnly[16];
	int rotationOnly[16];
	memcpy(translationOnly, s_identity, sizeof(translationOnly));
	memcpy(rotationOnly, globalTransformation, sizeof(rotationOnly));

	translationOnly[3] = globalTransformation[3];
	rotationOnly[3] = 0;
	translationOnly[7] = globalTransformation[7];
	rotationOnly[7] = 0;
	translationOnly[11] = globalTransformation[11];
	rotationOnly[11] = 0;

	int m[16];
	int m2[16];
	memcpy(m, s_identity, sizeof(m));
	memcpy(m2, s_identity, sizeof(m));

	//MATRIX_MUL_TO(globalTransformation, 4, 4, translationOnly, 4, 4, m);
	MATRIX_MUL_TO(translationOnly, 4, 4, rotationOnly, 4, 4, m2);



	MATRIX_MUL_TO(m2, 4, 4, pos, 4, 1, result);
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
 */

#define MATRIXARRAY_SET_ROTATEX(_Degrees, _Matrix_16floats){ \
		float* _m = _Matrix_16floats,_cx=cos((_Degrees)*0.0174532925f),_sx=sin((_Degrees)*0.0174532925f);_m[0]=1;_m[1]=0;_m[2]=0;_m[3]=0;_m[4]=0;_m[5]=_cx;_m[6]=_sx;_m[7]=0;_m[8]=0;_m[9]=-_sx;_m[10]=_cx;_m[11]=0;_m[12]=0;_m[13]=0;_m[14]=0;_m[15]=1;}


void Node::transformGlobal(int x, int y, int z, int* pX, int* pY, int* pZ, bool snapToVoxel) const
{
	int globalTransformation[16];

	getGlobalTransformation(globalTransformation);
	//getLocalTransformation(globalTransformation);
/*
	int globalTranslation[16];
	memcpy(globalTranslation, s_identity, sizeof(globalTranslation));
	globalTranslation[3] = globalTransformation[3];
	globalTranslation[7] = globalTransformation[7];
	globalTranslation[11] = globalTransformation[11];

	int globalRotation[16];
	memcpy(globalRotation, globalTransformation, sizeof(globalRotation));
	globalRotation[3] = 0;
	globalRotation[7] = 0;
	globalRotation[11] = 0;
*/

	float pos[4] = {(float)x, (float)y, (float)z, 1};
	float result[4] = {0, 0, 0, 0};

	if(snapToVoxel)
	{
		pos[0] += 0.5f;
		pos[1] += 0.5f;
		pos[2] += 0.5f;
	}

	//int temp1[16];
	//MATRIX_MUL_TO(globalTranslation, 4, 4, globalRotation, 4, 4, temp1);
	//MATRIX_MUL_TO(globalRotation, 4, 4, pos, 4, 1, result);

	float globalTransformationF[16];
	for(size_t i=0; i<16; i++)
	{
		globalTransformationF[i] = (float)globalTransformation[i];
	}

	MATRIX_MUL_TO(globalTransformation, 4, 4, pos, 4, 1, result);
	//MATRIX_MUL_TO(pos, 1, 4, globalTransformation, 4, 4, result);

	// different aproach: follow the 'forward' (z) axis instead of playing with vector transformations

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


