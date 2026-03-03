#include "GGeometry.h"

GCone::GCone(const GPoint & ptBtmCenter, const double radius, const double height,
			 const double halfAngle)
	: m_TopH(ptBtmCenter.position.z + height), m_BtmH(ptBtmCenter.position.z),
	  m_HalfAngle(halfAngle) {
	m_PtApex = {ptBtmCenter};
	m_PtApex.position.z += radius / std::tan(halfAngle);
}

std::vector<GCurve *> GCone::getCircle(const double height) const noexcept {
	if (compleDouble(height, m_PtApex.position.z) == 0)
		return {};

	GVector vecBegin{GVector::xAxis};
	// 暂时不用
	/* if (compleDouble(height, m_PtApex.position.z) > 0)
		vecBegin *= -1; */

	const GVector vecEnd{vecBegin * -1};

	const GPoint ptCenter{m_PtApex.position.x, m_PtApex.position.y, height};

	const double & raduis = std::tan(m_HalfAngle) * std::abs(m_PtApex.position.z - height);

	return std::vector<GCurve *>{
		new GArc{ptCenter, raduis, m_VecAxis, vecBegin, vecEnd},
		new GArc{ptCenter, raduis, m_VecAxis, vecEnd, vecBegin},
	};
}

VertexInfo GCone::getVertex() const noexcept {
	const glm::vec3 & color = getDebugColor(m_PtApex.position);

	VertexInfo vertInfo;

	if ((compleDouble(m_PtApex.position.z, m_TopH) <= 0) ||
		(compleDouble(m_PtApex.position.z, m_BtmH) >= 0)) {
		// 双锥
		bool hasBtm = compleDouble(m_PtApex.position.z, m_BtmH) != 0;
		bool hasTop = compleDouble(m_PtApex.position.z, m_TopH) != 0;

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		if (!hasBtm && !hasTop)
			return {};
		else if (hasBtm && hasTop) {
			vertexCount = (36 + 36 / 2) + (36 + 1);
			indexCount = 36 * 2 * 2 * 3;
		} else {
			vertexCount = 36 + 36 / 2;
			indexCount = 36 * 2 * 3;
		}

		vertInfo.vec_Vertex.reserve(vertexCount);
		vertInfo.vec_Vertex.reserve(indexCount);

		Vertex vertApex{
			.pos = m_PtApex.to_GlmVec3(),
			.color = color,
		};

		if (hasBtm) {
			// 底部
			std::vector<GCurve *> vec_BtmCurve = getCircle(m_BtmH);
			std::vector<GPoint> vec_PtVert{36};

			for (const auto & crv : vec_BtmCurve) {
				const auto & vec_PtVertTmp = crv->getVertex();
				for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i)
					vec_PtVert.emplace_back(vec_PtVertTmp[i]);
			}

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_PtApex.position.y}
						   .to_GlmVec3(),
				.normal = (m_VecAxis * -1).to_GlmVec3(),
				.color = color,
			});

			const uint32_t & btmCenterPtIndex =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

a
			for (size_t currPtIndex = 0; currPtIndex < vec_PtVert.size(); ++currPtIndex) {
				const size_t & nextPtIndex = currPtIndex + 1;
				a
			}

			for (auto & p : vec_BtmCurve)
				delete p;
		}
		if (hasTop) {
			// 顶部
		}

	} else {
	}

	return vertInfo;
}