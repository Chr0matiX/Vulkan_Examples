#include "GGeometry.h"

#include <cassert>
#include <optional>

GCone::GCone(const GPoint & ptBtmCenter, const double radius, const double height,
			 const double halfAngle)
	: m_HalfAngle(halfAngle) {

	if (halfAngle < 0) {
		m_TopH = ptBtmCenter.position.z;
		m_BtmH = ptBtmCenter.position.z - height;
	} else {
		m_TopH = ptBtmCenter.position.z + height;
		m_BtmH = ptBtmCenter.position.z;
	}

	m_PtApex = {ptBtmCenter};
	m_PtApex.position.z += (radius / std::tan(glm::radians(halfAngle)));
}

std::vector<std::unique_ptr<GCurve>> GCone::getCircle(const double height) const noexcept {
	if (compareDouble(height, m_PtApex.position.z) == 0)
		return {};

	GVector vecBegin{GVector::xAxis};
	// 暂时不用
	/* if (compareDouble(height, m_PtApex.position.z) > 0)
		vecBegin *= -1; */

	const GVector vecEnd{vecBegin * -1};

	const GPoint ptCenter{m_PtApex.position.x, m_PtApex.position.y, height};

	const double & raduis =
		std::tan(glm::radians(m_HalfAngle)) * std::abs(m_PtApex.position.z - height);

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

	if ((compareDouble(m_PtApex.position.z, m_TopH) <= 0) &&
		(compareDouble(m_PtApex.position.z, m_BtmH) >= 0)) {
		// 双锥
		bool hasBtm = compareDouble(m_PtApex.position.z, m_BtmH) != 0;
		bool hasTop = compareDouble(m_PtApex.position.z, m_TopH) != 0;

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

			std::optional<uint32_t> idxVertFst;
			uint32_t idxVertFstBtm{0};
			GVector vecVertFst;

			uint32_t idxVertPre{0};
			uint32_t idxVertPreBtm{0};
			GVector vecVertPre;

			for (const auto & crv : vec_BtmCurve) {
				const auto & vec_PtVertTmp = crv->getVertex();

				for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
					const auto & ptCurr = vec_PtVertTmp[i];
					const auto & vecTan = crv->getTangentAt(ptCurr);
					const auto & vecVertNormal = vecTan.cross(m_PtApex - ptCurr).normalize();

					// 侧面圆周当前顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecVertNormal.to_GlmVec3(),
						.color = color,
					});
					const uint32_t & idxVertCurr =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					// 底面圆周当前顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecBtmNormal,
						.color = color,
					});
					const uint32_t & idxVertCurrBtm =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					if (!idxVertFst.has_value()) {
						idxVertFst = idxVertCurr;
						idxVertFstBtm = idxVertCurrBtm;
						vecVertFst = vecVertNormal;

						idxVertPre = idxVertCurr;
						idxVertPreBtm = idxVertCurrBtm;
						vecVertPre = vecVertNormal;
						continue;
					}

					// 锥顶顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = m_PtApex.to_GlmVec3(),
						.normal = (vecVertPre + vecVertNormal).normalize().to_GlmVec3(),
						.color = color,
					});
					const uint32_t & idxVertCurrApex =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					vertInfo.vec_Index.emplace_back(idxVertPre);
					vertInfo.vec_Index.emplace_back(idxVertCurr);
					vertInfo.vec_Index.emplace_back(idxVertCurrApex);

					vertInfo.vec_Index.emplace_back(idxVertCurrBtm);
					vertInfo.vec_Index.emplace_back(idxVertPreBtm);
					vertInfo.vec_Index.emplace_back(idxBtmCenterVert);

					idxVertPre = idxVertCurr;
					idxVertPreBtm = idxVertCurrBtm;
					vecVertPre = vecVertNormal;
				}
			}

			// last
			if (idxVertFst != idxVertPre) {
				vertInfo.vec_Vertex.emplace_back(Vertex{
					.pos = m_PtApex.to_GlmVec3(),
					.normal = (vecVertPre + vecVertFst).to_GlmVec3(),
					.color = color,
				});

				vertInfo.vec_Index.emplace_back(idxVertPre);
				vertInfo.vec_Index.emplace_back(idxVertFst.value());
				vertInfo.vec_Index.emplace_back(
					static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1));

				vertInfo.vec_Index.emplace_back(idxVertFstBtm);
				vertInfo.vec_Index.emplace_back(idxVertPreBtm);
				vertInfo.vec_Index.emplace_back(idxBtmCenterVert);
			}
		}

		if (hasTop) {
			// 顶部
			const auto & vecTopNormal = m_VecAxis.to_GlmVec3();

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_TopH}.to_GlmVec3(),
				.normal = vecTopNormal,
				.color = color,
			});

			const uint32_t & idxTopCenterVert =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			std::vector<std::unique_ptr<GCurve>> vec_TopCurve = getCircle(m_TopH);

			std::optional<uint32_t> idxVertFst;
			uint32_t idxVertFstTop{0};
			GVector vecVertFst;

			uint32_t idxVertPre{0};
			uint32_t idxVertPreTop{0};
			GVector vecVertPre;

			for (const auto & crv : vec_TopCurve) {
				const auto & vec_PtVertTmp = crv->getVertex();

				for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
					const auto & ptCurr = vec_PtVertTmp[i];
					const auto & vecTan = crv->getTangentAt(ptCurr);
					const auto & vecVertNormal = vecTan.cross(ptCurr - m_PtApex).normalize();

					// 侧面圆周当前顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecVertNormal.to_GlmVec3(),
						.color = color,
					});
					const uint32_t & idxVertCurr =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					// 顶面圆周当前顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = ptCurr.to_GlmVec3(),
						.normal = vecTopNormal,
						.color = color,
					});
					const uint32_t & idxVertCurrTop =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					if (!idxVertFst.has_value()) {
						idxVertFst = idxVertCurr;
						idxVertFstTop = idxVertCurrTop;
						vecVertFst = vecVertNormal;

						idxVertPre = idxVertCurr;
						idxVertPreTop = idxVertCurrTop;
						vecVertPre = vecVertNormal;
						continue;
					}

					// 锥顶顶点
					vertInfo.vec_Vertex.emplace_back(Vertex{
						.pos = m_PtApex.to_GlmVec3(),
						.normal = (vecVertPre + vecVertNormal).normalize().to_GlmVec3(),
						.color = color,
					});
					const uint32_t & idxVertCurrApex =
						static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

					vertInfo.vec_Index.emplace_back(idxVertCurr);
					vertInfo.vec_Index.emplace_back(idxVertPre);
					vertInfo.vec_Index.emplace_back(idxVertCurrApex);

					vertInfo.vec_Index.emplace_back(idxVertPreTop);
					vertInfo.vec_Index.emplace_back(idxVertCurrTop);
					vertInfo.vec_Index.emplace_back(idxTopCenterVert);

					idxVertPre = idxVertCurr;
					idxVertPreTop = idxVertCurrTop;
					vecVertPre = vecVertNormal;
				}
			}

			// last
			if (idxVertFst != idxVertPre) {
				vertInfo.vec_Vertex.emplace_back(Vertex{
					.pos = m_PtApex.to_GlmVec3(),
					.normal = (vecVertPre + vecVertFst).to_GlmVec3(),
					.color = color,
				});

				vertInfo.vec_Index.emplace_back(idxVertFst.value());
				vertInfo.vec_Index.emplace_back(idxVertPre);
				vertInfo.vec_Index.emplace_back(
					static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1));

				vertInfo.vec_Index.emplace_back(idxVertPreTop);
				vertInfo.vec_Index.emplace_back(idxVertFstTop);
				vertInfo.vec_Index.emplace_back(idxTopCenterVert);
			}
		}
	} else {
		// 圆台
		if (compareDouble(m_TopH, m_BtmH) == 0)
			return {};

		vertInfo.vec_Vertex.reserve(36 * 2 + 2);
		vertInfo.vec_Index.reserve(36 * 2 * 2 * 3);

		std::vector<std::pair<GPoint, GVector>> vec_Pt2VecTop;
		std::vector<std::pair<GPoint, GVector>> vec_Pt2VecBtm;

		for (const auto & crv : getCircle(m_TopH)) {
			const auto & vec_PtVertTmp = crv->getVertex();

			for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
				const auto & ptCurr = vec_PtVertTmp[i];
				const auto & vecTan = crv->getTangentAt(ptCurr);
				const auto & vecVertNormal = vecTan.cross(m_PtApex - ptCurr).normalize();

				vec_Pt2VecTop.emplace_back(ptCurr, vecVertNormal);
			}
		}

		for (const auto & crv : getCircle(m_BtmH)) {
			const auto & vec_PtVertTmp = crv->getVertex();

			for (size_t i = 0; i < vec_PtVertTmp.size() - 1; ++i) {
				const auto & ptCurr = vec_PtVertTmp[i];
				const auto & vecTan = crv->getTangentAt(ptCurr);
				const auto & vecVertNormal = vecTan.cross(m_PtApex - ptCurr).normalize();

				vec_Pt2VecBtm.emplace_back(ptCurr, vecVertNormal);
			}
		}

		assert(vec_Pt2VecTop.size() == vec_Pt2VecBtm.size());

		const auto & vecTopNormal = m_VecAxis.to_GlmVec3();
		const auto & vecBtmNormal = (m_VecAxis * -1).to_GlmVec3();

		vertInfo.vec_Vertex.emplace_back(Vertex{
			.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_TopH}.to_GlmVec3(),
			.normal = vecTopNormal,
			.color = color,
		});
		const uint32_t & idxTopCenterVert = static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

		vertInfo.vec_Vertex.emplace_back(Vertex{
			.pos = GPoint{m_PtApex.position.x, m_PtApex.position.y, m_BtmH}.to_GlmVec3(),
			.normal = vecBtmNormal,
			.color = color,
		});
		const uint32_t & idxBtmCenterVert = static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

		std::optional<uint32_t> idxVertFstTopSide;
		uint32_t idxVertFstBtmSide{0};
		uint32_t idxVertFstTop{0};
		uint32_t idxVertFstBtm{0};

		uint32_t idxVertPreTopSide;
		uint32_t idxVertPreBtmSide{0};
		uint32_t idxVertPreTop{0};
		uint32_t idxVertPreBtm{0};

		for (size_t i = 0; i < vec_Pt2VecTop.size(); ++i) {
			const auto & ptCurrTop = vec_Pt2VecTop[i].first;
			const auto & ptCurrBtm = vec_Pt2VecBtm[i].first;
			const auto & vecCurrTop = vec_Pt2VecTop[i].second;
			const auto & vecCurrBtm = vec_Pt2VecBtm[i].second;

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = ptCurrTop.to_GlmVec3(),
				.normal = vecCurrTop.to_GlmVec3(),
				.color = color,
			});
			const uint32_t & idxVertCurrTopSide =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = ptCurrBtm.to_GlmVec3(),
				.normal = vecCurrBtm.to_GlmVec3(),
				.color = color,
			});
			const uint32_t & idxVertCurrBtmSide =
				static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = ptCurrTop.to_GlmVec3(),
				.normal = vecTopNormal,
				.color = color,
			});
			const uint32_t & idxVertCurrTop = static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			vertInfo.vec_Vertex.emplace_back(Vertex{
				.pos = ptCurrBtm.to_GlmVec3(),
				.normal = vecBtmNormal,
				.color = color,
			});
			const uint32_t & idxVertCurrBtm = static_cast<uint32_t>(vertInfo.vec_Vertex.size() - 1);

			if (!idxVertFstTopSide.has_value()) {
				idxVertFstTopSide = idxVertCurrTopSide;
				idxVertFstBtmSide = idxVertCurrBtmSide;
				idxVertFstTop = idxVertCurrTop;
				idxVertFstBtm = idxVertCurrBtm;

				idxVertPreTopSide = idxVertCurrTopSide;
				idxVertPreBtmSide = idxVertCurrBtmSide;
				idxVertPreTop = idxVertCurrTop;
				idxVertPreBtm = idxVertCurrBtm;

				continue;
			}

			vertInfo.vec_Index.emplace_back(idxVertCurrTopSide);
			vertInfo.vec_Index.emplace_back(idxVertPreTopSide);
			vertInfo.vec_Index.emplace_back(idxVertPreBtmSide);

			vertInfo.vec_Index.emplace_back(idxVertCurrTopSide);
			vertInfo.vec_Index.emplace_back(idxVertPreBtmSide);
			vertInfo.vec_Index.emplace_back(idxVertCurrBtmSide);

			vertInfo.vec_Index.emplace_back(idxVertPreTop);
			vertInfo.vec_Index.emplace_back(idxVertCurrTop);
			vertInfo.vec_Index.emplace_back(idxTopCenterVert);

			vertInfo.vec_Index.emplace_back(idxVertCurrBtm);
			vertInfo.vec_Index.emplace_back(idxVertPreBtm);
			vertInfo.vec_Index.emplace_back(idxBtmCenterVert);

			idxVertPreTopSide = idxVertCurrTopSide;
			idxVertPreBtmSide = idxVertCurrBtmSide;
			idxVertPreTop = idxVertCurrTop;
			idxVertPreBtm = idxVertCurrBtm;
		}

		// last
		if (idxVertFstTopSide != idxVertPreTopSide) {
			vertInfo.vec_Index.emplace_back(idxVertFstTopSide.value());
			vertInfo.vec_Index.emplace_back(idxVertPreTopSide);
			vertInfo.vec_Index.emplace_back(idxVertPreBtmSide);

			vertInfo.vec_Index.emplace_back(idxVertFstTopSide.value());
			vertInfo.vec_Index.emplace_back(idxVertPreBtmSide);
			vertInfo.vec_Index.emplace_back(idxVertFstBtmSide);

			vertInfo.vec_Index.emplace_back(idxVertPreTop);
			vertInfo.vec_Index.emplace_back(idxVertFstTop);
			vertInfo.vec_Index.emplace_back(idxTopCenterVert);

			vertInfo.vec_Index.emplace_back(idxVertFstBtm);
			vertInfo.vec_Index.emplace_back(idxVertPreBtm);
			vertInfo.vec_Index.emplace_back(idxBtmCenterVert);
		}
	}

	return vertInfo;
}

GPoint GCone::getFeaturePt() const noexcept {
	return GPoint{m_PtApex.to_GlmVec3().x, m_PtApex.to_GlmVec3().y, (m_TopH + m_BtmH) / 2};
}