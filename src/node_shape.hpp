
#ifndef VOXTOPNG_NODE_SHAPE_HPP
#define VOXTOPNG_NODE_SHAPE_HPP

#include <memory>
#include <vector>
#include "node.hpp"

class Model;
class Scene;
class Color;

class NodeShapeModel
{
	friend class Scene;
	public:
		NodeShapeModel(const Context& context);
		int32_t 				getId() const;
		error 					load(Loader& loader);
		const weak_ptr<Model>& 	getModel() const;

	protected:
		void 					setModel(const weak_ptr<Model>& pModel);

	private:
		int32_t 		m_id;
		weak_ptr<Model>	m_pModel;
		Dictionary 		m_attributes;
};

class NodeShape : public Node
{
	public:
		explicit NodeShape(const Context& context);
		error 				load(Loader& loader) override;
		const NodeShape*	toNodeShape() const override { return this; };
		const Color*		getVoxel(int x, int y, int z) const;
		void 				getSize(int32_t* pX, int32_t* pY, int32_t* pZ) const;

	private:
		vector<shared_ptr<NodeShapeModel>> models;
};


#endif //VOXTOPNG_NODE_SHAPE_HPP
