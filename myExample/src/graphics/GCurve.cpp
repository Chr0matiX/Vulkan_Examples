#include "GCurve.h"

#include "../Utils.hpp"

std::vector<GPoint> GArc::getVertex() const noexcept {
	const uint8_t maxFragCount = 18;
	const double fragAngle = m_VecBegin.angleTo(m_VecEnd, m_VecNormal) / maxFragCount;
	if (compleDouble(fragAngle, 0, 0.1) <= 0)
		return {};

	std::vector<GPoint> vec_Pt;
	for (uint8_t i = 0; i < maxFragCount; ++i) {
		GPoint ptTmp{m_PtCenter};
		GVector vecTmp{m_VecBegin};

		ptTmp +=
			vecTmp.transformBy(GMatrix().setToRotate(m_PtCenter, m_VecNormal, (i + 1) * fragAngle));

		vec_Pt.emplace_back(ptTmp);
	}

	return vec_Pt;
}