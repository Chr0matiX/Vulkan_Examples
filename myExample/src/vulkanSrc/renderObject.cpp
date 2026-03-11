#include "renderObject.h"

#include "../graphics/GGeometry.h"

RenderObject::~RenderObject() {
	if (m_pGeo != nullptr) {
		delete m_pGeo;
		m_pGeo = nullptr;
	}
}

VertexInfo & RenderObject::getVertexInfo() noexcept {
	if (!bInit) {
		m_VertInfo = std::move(m_pGeo->getVertex());
		bInit = true;
	}

	return m_VertInfo;
}

GPoint RenderObject::getFeaturePt() const noexcept {
	return m_pGeo->getFeaturePt();
}

RenderObjectManager * RenderObjectManager::m_pInstance{nullptr};

RenderObjectManager & RenderObjectManager::getInstance() {
	if (m_pInstance == nullptr)
		m_pInstance = new RenderObjectManager();

	return *m_pInstance;
}

void RenderObjectManager::destroy() {
	for (auto & pRenderObj : vec_pRenderObj)
		delete pRenderObj;

	delete m_pInstance;
}

size_t RenderObjectManager::addRenderObject(RenderObject * const pRenderObj) noexcept {
	if (pRenderObj == nullptr)
		return -1;

	m_Update = false;

	vec_pRenderObj.emplace_back(pRenderObj);

	return vec_pRenderObj.size() - 1;
}

bool RenderObjectManager::update() {
	if (m_Update)
		return true;

	if (vec_pRenderObj.empty())
		return true;

	size_t totalVertices = 0;
	size_t totalIndices = 0;
	for (const auto & pObj : vec_pRenderObj) {
		totalVertices += pObj->getVertexInfo().vec_Vertex.size();
		totalIndices += pObj->getVertexInfo().vec_Index.size();
	}

	m_VertInfoMerg.vec_Vertex.clear();
	m_VertInfoMerg.vec_Index.clear();
	m_VertInfoMerg.vec_Vertex.reserve(totalVertices);
	m_VertInfoMerg.vec_Index.reserve(totalIndices);

	uint32_t vertCount{0};
	uint32_t indexCount{0};
	for (const auto & pRenderObj : vec_pRenderObj) {
		const auto & vertInfo = pRenderObj->getVertexInfo();

		m_VertInfoMerg.vec_Vertex.insert(m_VertInfoMerg.vec_Vertex.end(),
										 vertInfo.vec_Vertex.begin(), vertInfo.vec_Vertex.end());
		m_VertInfoMerg.vec_Index.insert(m_VertInfoMerg.vec_Index.end(), vertInfo.vec_Index.begin(),
										vertInfo.vec_Index.end());

		pRenderObj->setVertexOffset(vertCount);
		pRenderObj->setIndexOffset(indexCount);
		pRenderObj->setIndexCount(static_cast<uint32_t>(vertInfo.vec_Index.size()));

		vertCount += vertInfo.vec_Vertex.size();
		indexCount += vertInfo.vec_Index.size();
	}

	m_Update = true;

	return true;
}

const std::vector<const RenderObject *>
RenderObjectManager::getVecRenderObj(const std::function<bool(const GPoint &)> func) {
	if (!m_Update && update())
		return {};

	std::vector<const RenderObject *> vec_pRtnRenderObj;
	// vec_RenderObj.reserve(vec_pRenderObj.size() / 2);
	vec_pRtnRenderObj.reserve(vec_pRenderObj.size());

	for (const auto & pRenderObj : vec_pRenderObj) {
		if (func(pRenderObj->getFeaturePt()))
			vec_pRtnRenderObj.emplace_back(pRenderObj);
	}

	return vec_pRtnRenderObj;
}