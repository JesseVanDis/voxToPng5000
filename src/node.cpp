
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


