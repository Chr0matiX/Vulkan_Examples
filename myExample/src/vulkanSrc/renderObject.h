#pragma once

#include "../Utils.hpp"

#include <functional>
#include <map>

class GGeometry;
class RenderObjectManager;
class GPoint;

class RenderObject {
		// friend RenderObjectManager;

	private:
		const GGeometry * m_pGeo{nullptr};

	private:
		uint32_t m_IndexCount{0};
		uint32_t m_IndexOffset{0};
		uint32_t m_VertexOffset{0};

		VertexInfo m_VertInfo;

		bool bInit{false};

	public:
		RenderObject(const GGeometry * const pGeo) : m_pGeo(pGeo) {}
		~RenderObject();

		void setIndexCount(const uint32_t value) noexcept { m_IndexCount = value; }
		void setVertexOffset(const uint32_t value) noexcept { m_VertexOffset = value; }
		void setIndexOffset(const uint32_t value) noexcept { m_IndexOffset = value; }
		uint32_t getIndexCount() const noexcept { return m_IndexCount; }
		uint32_t getVertexOffset() const noexcept { return m_VertexOffset; }
		uint32_t getIndexOffset() const noexcept { return m_IndexOffset; }

		VertexInfo & getVertexInfo() noexcept;

		GPoint getFeaturePt() const noexcept;
};

class RenderObjectManager {
		SINGLETON_CLASS(RenderObjectManager);

	private:
		static RenderObjectManager * m_pInstance;

		bool m_Update{false};

		std::vector<RenderObject *> vec_pRenderObj;

		VertexInfo m_VertInfoMerg;

	private:
		bool update();

	public:
		static RenderObjectManager & getInstance();

		void destroy();

		size_t addRenderObject(RenderObject * const pRenderObj) noexcept;

		const std::vector<const RenderObject *>
		getVecRenderObj(const std::function<bool(const GPoint &)> func);

		VertexInfo & getVertexInfo() { return m_VertInfoMerg; }

		uint32_t getRenderObjCount() { return static_cast<uint32_t>(vec_pRenderObj.size()); }
};