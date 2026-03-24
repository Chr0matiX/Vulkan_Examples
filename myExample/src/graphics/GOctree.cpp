#include "GOctree.h"

#include "../Utils.hpp"

bool AABB::isPtIn(const GPoint & pt, const double tol) const noexcept {
	return (compareDouble(pt.position.x, m_MaxPt.position.x, tol) <= 0) &&
		   (compareDouble(pt.position.x, m_MinPt.position.x, tol) >= 0) &&

		   (compareDouble(pt.position.y, m_MaxPt.position.y, tol) <= 0) &&
		   (compareDouble(pt.position.y, m_MinPt.position.y, tol) >= 0) &&

		   (compareDouble(pt.position.z, m_MaxPt.position.z, tol) <= 0) &&
		   (compareDouble(pt.position.z, m_MinPt.position.z, tol) >= 0);
}

bool AABB::isIntersect(const AABB & othAABB, const double tol) const noexcept {
	const bool & xNotOverlap =
		compareDouble(m_MinPt.position.x, othAABB.m_MaxPt.position.x, tol) > 0 ||
		compareDouble(othAABB.m_MinPt.position.x, m_MaxPt.position.x, tol) > 0;

	const bool & yNotOverlap =
		compareDouble(m_MinPt.position.y, othAABB.m_MaxPt.position.y, tol) > 0 ||
		compareDouble(othAABB.m_MinPt.position.y, m_MaxPt.position.y, tol) > 0;

	const bool & zNotOverlap =
		compareDouble(m_MinPt.position.z, othAABB.m_MaxPt.position.z, tol) > 0 ||
		compareDouble(othAABB.m_MinPt.position.z, m_MaxPt.position.z, tol) > 0;

	return !(xNotOverlap || yNotOverlap || zNotOverlap);
}

void AABB::set(const GPoint & maxPt, const GPoint & minPt) noexcept {
	m_MaxPt = maxPt;
	m_MinPt = minPt;
}

void AABB::add(const AABB & othAABB) noexcept {

	if (compareDouble(othAABB.m_MinPt.position.x, m_MinPt.position.x) < 0)
		m_MinPt.position.x = othAABB.m_MinPt.position.x;
	if (compareDouble(othAABB.m_MinPt.position.y, m_MinPt.position.y) < 0)
		m_MinPt.position.y = othAABB.m_MinPt.position.y;
	if (compareDouble(othAABB.m_MinPt.position.z, m_MinPt.position.z) < 0)
		m_MinPt.position.z = othAABB.m_MinPt.position.z;

	if (compareDouble(othAABB.m_MaxPt.position.x, m_MaxPt.position.x) > 0)
		m_MaxPt.position.x = othAABB.m_MaxPt.position.x;
	if (compareDouble(othAABB.m_MaxPt.position.y, m_MaxPt.position.y) > 0)
		m_MaxPt.position.y = othAABB.m_MaxPt.position.y;
	if (compareDouble(othAABB.m_MaxPt.position.z, m_MaxPt.position.z) > 0)
		m_MaxPt.position.z = othAABB.m_MaxPt.position.z;
}

void AABB::add(const GPoint & pt) noexcept {
	if (compareDouble(pt.position.x, m_MinPt.position.x) < 0)
		m_MinPt.position.x = pt.position.x;
	if (compareDouble(pt.position.y, m_MinPt.position.y) < 0)
		m_MinPt.position.y = pt.position.y;
	if (compareDouble(pt.position.z, m_MinPt.position.z) < 0)
		m_MinPt.position.z = pt.position.z;

	if (compareDouble(pt.position.x, m_MaxPt.position.x) > 0)
		m_MaxPt.position.x = pt.position.x;
	if (compareDouble(pt.position.y, m_MaxPt.position.y) > 0)
		m_MaxPt.position.y = pt.position.y;
	if (compareDouble(pt.position.z, m_MaxPt.position.z) > 0)
		m_MaxPt.position.z = pt.position.z;
}

GPoint AABB::getCenter() const noexcept {
	return GPoint((m_MinPt.position.x + m_MaxPt.position.x) * 0.5,
				  (m_MinPt.position.y + m_MaxPt.position.y) * 0.5,
				  (m_MinPt.position.z + m_MaxPt.position.z) * 0.5);
}

AABB AABB::getChildAABB(const uint8_t index) const noexcept {
	GPoint center = getCenter();
	GPoint subMin, subMax;

	if (index & 1) {
		subMin.position.x = center.position.x;
		subMax.position.x = m_MaxPt.position.x;
	} else {
		subMin.position.x = m_MinPt.position.x;
		subMax.position.x = center.position.x;
	}

	if (index & 2) {
		subMin.position.y = center.position.y;
		subMax.position.y = m_MaxPt.position.y;
	} else {
		subMin.position.y = m_MinPt.position.y;
		subMax.position.y = center.position.y;
	}

	if (index & 4) {
		subMin.position.z = center.position.z;
		subMax.position.z = m_MaxPt.position.z;
	} else {
		subMin.position.z = m_MinPt.position.z;
		subMax.position.z = center.position.z;
	}

	AABB res;
	res.set(subMax, subMin);
	return res;
}