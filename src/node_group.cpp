
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
	for(int32_t i=0; i<numNodes; i++)
	{
		NodeChild child(this, getContext());
		err = child.load(loader);
		if(err != ""s)
		{
			return err;
		}
		m_children.push_back(child);
	}

	return ""s;
}
