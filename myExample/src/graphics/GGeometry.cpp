#include "GGeometry.h"

#include <optional>

GCone::GCone(const GPoint & ptBtmCenter, const double radius, const double height,
			 const double halfAngle)
	: m_TopH(ptBtmCenter.position.z + height), m_BtmH(ptBtmCenter.position.z),
	  m_HalfAngle(halfAngle) {
	m_PtApex = {ptBtmCenter};
	m_PtApex.position.z += radius / std::tan(halfAngle);
}

std::vector<std::unique_ptr<GCurve>> GCone::getCircle(const double height) const noexcept {
	if (compleDouble(height, m_PtApex.position.z) == 0)
		return {};

	GVector vecBegin{GVector::xAxis};
	// 暂时不用
	/* if (compleDouble(height, m_PtApex.position.z) > 0)
		vecBegin *= -1; */

	const GVector vecEnd{vecBegin * -1};

	const GPoint ptCenter{m_PtApex.position.x, m_PtApex.position.y, height};

	const double & raduis = std::tan(m_HalfAngle) * std::abs(m_PtApex.position.z - height);

	std::vector<std::unique_ptr<GCurve>> vec_Rtn;
	vec_Rtn.reserve(2);

	vec_Rtn.emplace_back(
		std::move(std::make_unique<GArc>(ptCenter, raduis, m_VecAxis, vecBegin, vecEnd)));
	vec_Rtn.emplace_back(
		std::move(std::make_unique<GArc>(ptCenter, raduis, m_VecAxis, vecEnd, vecBegin)));

	return vec_Rtn;
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
		vertInfo.vec_Index.reserve(indexCount);

		if (hasBtm) {
			// 底部
			const auto & vecBtmNormal = (m_VecAxis * -1).to_GlmVec3();

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_BtmH}.to_GlmVec3(),
				.normal = vecBtmNormal,
				.color = color,
			});

			const uint32_t & idxBtmCenterVert =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			std::vector<std::unique_ptr<GCurve>> vec_BtmCurve = getCircle(m_BtmH);

			std::optional<uint32_t> idxFstVert;
			uint32_t idxFstBtmVert{0};
			// GPoint ptFstVert;
			GVector vecFstVert;

			uint32_t idxPreVert{0};
			uint32_t idxPreVertBtm{0};
			// GPoint ptPreVert;
			GVector vecPreVert;

			for (const auto & crv : vec_BtmCurve) {
				const auto & vec_PtVertTmp = crv->getVertex();

				for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
					const auto & ptCurr = vec_PtVertTmp[i];
					const auto & vecTan = crv->getTangentAt(ptCurr);
					const auto & vecVertNormal = vecTan.cross(m_PtApex - ptCurr).normalize();

					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecVertNormal.to_GlmVec3(),
						.color = color,
					});
					const uint32_t & idxCurrVert =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecBtmNormal,
						.color = color,
					});
					const uint32_t & idxCurrBtmVert =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					if (!idxFstVert.has_value()) {
						idxFstVert = idxCurrVert;
						idxFstBtmVert = idxCurrBtmVert;
						vecFstVert = vecVertNormal;

						idxPreVert = idxCurrVert;
						idxPreVertBtm = idxCurrBtmVert;
						vecPreVert = vecVertNormal;
						continue;
					}

					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = m_PtApex.to_GlmVec3(),
						.normal = (vecPreVert + vecVertNormal).normalize().to_GlmVec3(),
						.color = color,
					});

					const uint32_t & idxCurrApexVert =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					vertInfo.vec_Index.emplace_back(idxCurrVert);
					vertInfo.vec_Index.emplace_back(idxPreVert);
					vertInfo.vec_Index.emplace_back(idxCurrApexVert);

					vertInfo.vec_Index.emplace_back(idxPreVertBtm);
					vertInfo.vec_Index.emplace_back(idxCurrBtmVert);
					vertInfo.vec_Index.emplace_back(idxBtmCenterVert);

					idxPreVert = idxCurrVert;
					idxPreVertBtm = idxCurrBtmVert;
					vecPreVert = vecVertNormal;
				}
			}

			// last
			if (firstVertIndex != preVertIndex) {
				vertInfo.vec_Vertex.emplace_back(Vertex{
					.pos = m_PtApex.to_GlmVec3(),
					.normal = (vecPreVert + vecFirstVert).to_GlmVec3(),
					.color = color,
				});

				vertInfo.vec_Index.emplace_back(firstVertIndex.value());
				vertInfo.vec_Index.emplace_back(preVertIndex);
				vertInfo.vec_Index.emplace_back(
					static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1));

				vertInfo.vec_Index.emplace_back(preVertIndex);
				vertInfo.vec_Index.emplace_back(firstVertIndex.value());
				vertInfo.vec_Index.emplace_back(btmCenterVertIndex);
			}
		}

		if (hasTop) {
			// 顶部
			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_BtmH}.to_GlmVec3(),
				.normal = m_VecAxis.to_GlmVec3(),
				.color = color,
			});

			const uint32_t & topCenterVertIndex =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			std::vector<std::unique_ptr<GCurve>> vec_TopCurve = getCircle(m_BtmH);

			std::optional<uint32_t> firstVertIndex;
			GVector vecFirstVert;

			uint32_t preVertIndex{0};
			GVector vecPreVert;

			for (const auto & crv : vec_TopCurve) {
				const auto & vec_PtVertTmp = crv->getVertex();

				for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
					const auto & ptCurr = vec_PtVertTmp[i];
					const auto & vecTan = crv->getTangentAt(ptCurr);
					const auto & vecVertNormal = vecTan.cross(ptCurr - m_PtApex).normalize();

					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecVertNormal.to_GlmVec3(),
						.color = color,
					});

					const uint32_t & currVertIndex =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					if (!firstVertIndex.has_value()) {
						firstVertIndex = currVertIndex;
						vecFirstVert = vecVertNormal;

						preVertIndex = currVertIndex;
						vecPreVert = vecVertNormal;
						continue;
					}

					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = m_PtApex.to_GlmVec3(),
						.normal = (vecPreVert + vecVertNormal).to_GlmVec3(),
						.color = color,
					});

					vertInfo.vec_Index.emplace_back(currVertIndex);
					vertInfo.vec_Index.emplace_back(preVertIndex);
					vertInfo.vec_Index.emplace_back(currVertIndex + 1);

					vertInfo.vec_Index.emplace_back(preVertIndex);
					vertInfo.vec_Index.emplace_back(currVertIndex);
					vertInfo.vec_Index.emplace_back(topCenterVertIndex);

					preVertIndex = currVertIndex;
					vecPreVert = vecVertNormal;
				}
			}

			// last
			if (firstVertIndex != preVertIndex) {
				vertInfo.vec_Vertex.emplace_back(Vertex{
					.pos = m_PtApex.to_GlmVec3(),
					.normal = (vecPreVert + vecFirstVert).to_GlmVec3(),
					.color = color,
				});

				vertInfo.vec_Index.emplace_back(firstVertIndex.value());
				vertInfo.vec_Index.emplace_back(preVertIndex);
				vertInfo.vec_Index.emplace_back(
					static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1));

				vertInfo.vec_Index.emplace_back(preVertIndex);
				vertInfo.vec_Index.emplace_back(firstVertIndex.value());
				vertInfo.vec_Index.emplace_back(topCenterVertIndex);
			}
		}
	} else {
		// 圆台
		if (compleDouble(m_TopH, m_BtmH) == 0)
			return {};

		vertInfo.vec_Vertex.reserve(36 * 2 + 2);
		vertInfo.vec_Index.reserve(36 * 2 * 2 * 3);

		vertInfo.vec_Vertex.emplace_back(Vertex{
			.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_BtmH}.to_GlmVec3(),
			.normal = (m_VecAxis * -1).to_GlmVec3(),
			.color = color,
		});

		std::vector<std::pair<GPoint, GVector>> vec_Pt2Vec;
	}

	return vertInfo;
}