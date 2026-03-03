#include "GCurve.h"

#include "../Utils.hpp"

GPoint GLineSeg::getClosestPt(const GPoint & pt, const bool extend) const noexcept {
	double ratio = (pt - m_PtBegin).dot(m_PtEnd - m_PtBegin) / getLength();

	if (!extend) {
		if (compleDouble(ratio, 0) <= 0)
			return m_PtBegin;
		else if (compleDouble(ratio, 1) >= 1)
			return m_PtEnd;
	}

	return getPtAt(ratio);
}

GPoint GArc::getClosestPt(const GPoint & pt, const bool extend) const noexcept {
	GVector vecPt = (pt - m_PtCenter).normalize();

	if (!extend) {
		double anglePt = m_VecBegin.angleTo(vecPt);
		double angleThis = m_VecBegin.angleTo(m_VecEnd);
		double turning = 180 + angleThis / 2;

		if ((compleDouble(anglePt, angleThis) >= 0) && (compleDouble(anglePt, turning) <= 0))
			vecPt = m_VecEnd;
		else if (compleDouble(anglePt, turning) > 0)
			vecPt = m_VecBegin;
	}

	GPoint ptTmp{m_PtCenter};
	ptTmp += (vecPt * m_Radius);
	return ptTmp;
}

std::vector<GPoint> GArc::getVertex() const noexcept {
	const uint8_t maxFragCount = 18;
	const double fragAngle = m_VecBegin.angleTo(m_VecEnd, m_VecNormal) / maxFragCount;
	if (compleDouble(fragAngle, 0, 0.1) <= 0)
		return {};

	std::vector<GPoint> vec_Pt;
	for (uint8_t i = 0; i < maxFragCount + 1; ++i) {
		GPoint ptTmp{m_PtCenter};
		GVector vecTmp{m_VecBegin};

		ptTmp += vecTmp.transformBy(GMatrix().setToRotate(m_PtCenter, m_VecNormal, i * fragAngle));

		vec_Pt.emplace_back(ptTmp);
	}

	return vec_Pt;
}