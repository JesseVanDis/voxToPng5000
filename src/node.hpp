
#ifndef VOXTOPNG_NODE_HPP
#define VOXTOPNG_NODE_HPP

#include <cstdint>
#include "dictionary.hpp"
#include <vector>
#include <memory>
#include "common.hpp"

using namespace std;

class Scene;
class Loader;
class NodeGroup;
class NodeShape;
class NodeTransform;

struct Context
{
	Scene* pScene = nullptr;
};

class Node
{
	friend class Scene;
	public:
		explicit Node(const Context& context);
		virtual error 					load(Loader& loader);
		int32_t 						getId() const;
		string							getName(bool checkParentsAsWell) const;

		virtual const NodeGroup*		toNodeGroup() const { return nullptr; };
		virtual const NodeShape*		toNodeShape() const { return nullptr; };
		virtual const NodeTransform*	toNodeTransform() const { return nullptr; };

		const float*					getGlobalTransformation() const;

		virtual void 					makeDirty();

	protected:
		void 							transformGlobal(int x, int y, int z, int* pX, int* pY, int* pZ, bool snapToVoxel) const;
		void 							setParent(const Node* pParent);
		const Node*						getParent() const;
		const Context& 					getContext() const;

	private:
		vector<const Node*>				getHierarchy() const;

		const Node*	m_pParent = nullptr;
		Context 	m_context;
		int32_t 	m_id = ~0u;
		Dictionary 	m_dictionary;

		struct
		{
			bool		isDirty;
			float 		globalTransformation[16];
		}	mutable m_cache;
};

class NodeChild
{
	friend class Scene;
	public:
		explicit NodeChild(Node* pParent, const Context& context);
		error 					load(Loader& loader);
		int32_t 				getId() const;
		const weak_ptr<Node>& 	getNode() const;

	protected:
		void 					setNodePtr(const weak_ptr<Node>& pNode);

	private:
		Context 		m_context;
		int32_t 		m_id = ~0u;
		weak_ptr<Node> 	m_pNode;
};

#endif //VOXTOPNG_NODE_HPP
