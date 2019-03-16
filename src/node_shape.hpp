
#ifndef VOXTOPNG_NODE_SHAPE_HPP
#define VOXTOPNG_NODE_SHAPE_HPP

#include <climits>
#include <memory>
#include <vector>
#include "node.hpp"
#include "model.hpp"

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
		const Color*		getVoxelGlobal(int x, int y, int z, int32_t* pModel = nullptr) const;
		void 				getSize(int32_t* pX, int32_t* pY, int32_t* pZ) const;
		bool 				hasModelId(int32_t id) const;
		void 				getGlobalBounds(int32_t* pX0, int32_t* pY0, int32_t* pZ0, int32_t* pX1, int32_t* pY1, int32_t* pZ1) const;
		virtual void 		makeDirty();

	private:
		vector<shared_ptr<NodeShapeModel> > m_models;

		struct CachedVoxel_32i {
			int x = INT_MAX;
			int y = INT_MAX;
			int z = INT_MAX;
			const Color* pColor = nullptr;
		};

		struct CachedVoxel {
			int x = 0;
			int y = 0;
			int z = 0;
			const Color* pColor = nullptr;
		};

		struct CachedGlobalModel
		{
			NodeShapeModel* 	pModel = nullptr;
			int 				offsetX;
			int 				offsetY;
			int 				offsetZ;
			uint				width;
			uint				height;
			uint				depth;
			vector<CachedVoxel> globalVoxels;
		};

		struct
		{
			bool 						isDirty;
			vector<CachedGlobalModel> 	globallyTranslatedVoxels;
		} mutable m_cache;
};


#endif //VOXTOPNG_NODE_SHAPE_HPP
