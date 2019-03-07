
#include <assert.h>
#include "node_transform.hpp"
#include "loader.hpp"

NodeTransformFrame::NodeTransformFrame()
{
	for (int& v : m_transformation)
	{
		v = 0;
	}
	m_transformation[0] = 1;
	m_transformation[5] = 1;
	m_transformation[10] = 1;
	m_transformation[15] = 1;
}

error NodeTransformFrame::load(Loader& loader)
{
	error err = m_attributes.load(loader);
	if(err != ""s)
	{
		return err;
	}

	vector<int> t = m_attributes.getValues("_t"s, vector<int>{0, 0, 0});
	m_transformation[3] = t[0];
	m_transformation[7] = t[1];
	m_transformation[11] = t[2];

	// TODO: Fix rotation ( not sure if this is the correct approach so i will leave it here )
	//vector<char> rotData = m_attributes.getData("_r"s);
	//uint8_t rotByte = (uint8_t)(rotData.empty() ? '\0' : rotData[0]);

	const uint8_t rotByte = (uint8_t)m_attributes.getValue("_r", 0);

	if(rotByte != 0)
	{
		// row 1
		{
			const int sign = getBit(rotByte, 4) ? -1 : 1;
			if(!getBit(rotByte, 0) && !getBit(rotByte, 1))
			{
				m_transformation[0] = sign;
				m_transformation[1] = 0;
				m_transformation[2] = 0;
			}
			else if(getBit(rotByte, 0) && !getBit(rotByte, 1))
			{
				m_transformation[0] = 0;
				m_transformation[1] = sign;
				m_transformation[2] = 0;
			}
			else if(!getBit(rotByte, 0) && getBit(rotByte, 1))
			{
				m_transformation[0] = 0;
				m_transformation[1] = 0;
				m_transformation[2] = sign;
			}
			else
			{
				assert(false); // corrupt rotation ?
				// TODO: return an error when im sure it is
			}
		}

		// row 2
		{
			const int sign = getBit(rotByte, 5) ? -1 : 1;
			if(!getBit(rotByte, 2) && !getBit(rotByte, 3))
			{
				m_transformation[4] = sign;
				m_transformation[5] = 0;
				m_transformation[6] = 0;
			}
			else if(getBit(rotByte, 2) && !getBit(rotByte, 3))
			{
				m_transformation[4] = 0;
				m_transformation[5] = sign;
				m_transformation[6] = 0;
			}
			else if(!getBit(rotByte, 2) && getBit(rotByte, 3))
			{
				m_transformation[4] = 0;
				m_transformation[5] = 0;
				m_transformation[6] = sign;
			}
			else
			{
				assert(false); // corrupt rotation ?
				// TODO: return an error when im sure it is
			}
		}

		// row 3
		{
			const int sign = getBit(rotByte, 6) ? -1 : 1;
			if(m_transformation[0] == 0 && m_transformation[4] == 0)
			{
				m_transformation[8] = sign;
				m_transformation[9] = 0;
				m_transformation[10] = 0;
			}
			else if(m_transformation[1] == 0 && m_transformation[5] == 0)
			{
				m_transformation[8] = 0;
				m_transformation[9] = sign;
				m_transformation[10] = 0;
			}
			else if(m_transformation[2] == 0 && m_transformation[6] == 0)
			{
				m_transformation[9] = 0;
				m_transformation[10] = sign;
				m_transformation[11] = 0;
			}
		}
	}


	return ""s;
}

void NodeTransformFrame::getTranslation(int* pX, int* pY, int* pZ) const
{
	if(pX != nullptr)
	{
		*pX = m_transformation[3];
	}
	if(pY != nullptr)
	{
		*pY = m_transformation[7];
	}
	if(pZ != nullptr)
	{
		*pZ = m_transformation[11];
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
	m_frames.reserve(((size_t)numFrames)+1u);
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

