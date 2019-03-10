
#include "node_group.hpp"
#include "loader.hpp"

NodeGroup::NodeGroup(const Context& context)
	: Node(context)
{

}

error NodeGroup::load(Loader& loader)
{
	error err = Node::load(loader);
	if(err != ""s)
	{
		return err;
	}

	int32_t numNodes = loader.readNextInt32();
	m_children.reserve((size_t)numNodes);
	for(int32_t i=0; i<numNodes; i++)
	{
		shared_ptr<NodeChild> pChild(new NodeChild(this, getContext()));
		err = pChild->load(loader);
		if(err != ""s)
		{
			return err;
		}
		m_children.emplace_back(pChild);
	}

	return ""s;
}
