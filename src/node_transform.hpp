
#ifndef VOXTOPNG_NODE_TRANSFORM_HPP
#define VOXTOPNG_NODE_TRANSFORM_HPP

#include <vector>
#include "node.hpp"

class NodeTransformFrame
{
	public:
		NodeTransformFrame();
		error 	load(Loader& loader);

		void 	getTranslation(int* pX, int* pY, int* pZ) const;
		void 	setTranslation(int x, int y, int z);
		void 	getTransformation(int pBuffer[16]) const;
		void 	setTransformation(int pBuffer[16]);

	private:
		Dictionary m_attributes;
		int m_transformation[16];
};

class NodeTransform : public Node
{
	public:
		explicit NodeTransform(const Context& context);
		error 					load(Loader& loader) override;
		const NodeTransform*	toNodeTransform() const override { return this; };

		void 					getTranslation(int* pX, int* pY, int* pZ) const;
		void 					setTranslation(int x, int y, int z);
		void 					getTransformation(int pBuffer[16]) const;
		void 					setTransformation(int pBuffer[16]);

		void 					makeDirty() override;

	private:
		NodeTransformFrame*			getFrame();
		const NodeTransformFrame*	getFrame() const;

		shared_ptr<NodeChild>	m_pChild;
		int32_t 				m_reserved = ~0u;
		int32_t 				m_layerId = ~0u;
		int32_t 				m_numFrames = 0u;
		vector<NodeTransformFrame> m_frames;
};


#endif //VOXTOPNG_NODE_TRANSFORM_HPP
