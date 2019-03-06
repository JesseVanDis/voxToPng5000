
#include <assert.h>
#include "node_transform.hpp"
#include "loader.hpp"

NodeTransformFrame::NodeTransformFrame()
{

}

error NodeTransformFrame::load(Loader& loader)
{
	error err = m_attributes.load(loader);
	if(err != ""s)
	{
		return err;
	}

	vector<int> t = m_attributes.getValues("_t"s, vector<int>{0, 0, 0});
	m_translation.x = t[0];
	m_translation.y = t[1];
	m_translation.z = t[2];

	return ""s;
}

void NodeTransformFrame::getTranslation(int* pX, int* pY, int* pZ) const
{
	if(pX != nullptr)
	{
		*pX = m_translation.x;
	}
	if(pY != nullptr)
	{
		*pY = m_translation.y;
	}
	if(pZ != nullptr)
	{
		*pZ = m_translation.z;
	}
}

NodeTransform::NodeTransform(const Context& context)
	: Node(context)
	, m_child(this, context)
{

}

error NodeTransform::load(Loader& loader)
{
	string err = Node::load(loader);
	if(err != ""s)
	{
		return err;
	}

	err = m_child.load(loader);
	if(err != ""s)
	{
		return err;
	}

	m_reserved = loader.readNextInt32();
	m_layerId = loader.readNextInt32();

	int32_t numFrames = loader.readNextInt32();
	m_frames.reserve(numFrames+1);
	for(size_t i=0; i<numFrames; i++)
	{
		NodeTransformFrame frame;
		err = frame.load(loader);
		if(err != ""s)
		{
			return err;
		}
		m_frames.push_back(frame);
	}

	return ""s;
}

void NodeTransform::getTranslation(int* pX, int* pY, int* pZ) const
{
	assert(m_frames.size() == 1); // something changed in the .vox standard ( should normally have exacly 1 at all times )
	if(m_frames.size() != 1)
	{
		printf("WARNING: Found multiple transform frame attributes. positions of nodes may be incorrect");
	}
	if(!m_frames.empty())
	{
		m_frames[0].getTranslation(pX, pY, pZ);
	}
}

