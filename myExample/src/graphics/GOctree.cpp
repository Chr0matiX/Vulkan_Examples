#include "GOctree.h"

#include "../Utils.hpp"

bool AABB::isPtIn(const GPoint & pt, const double tol) {
	return (compareDouble(pt.position.x, m_MaxPt.position.x, tol) <= 0) &&
		   (compareDouble(pt.position.x, m_MinPt.position.x, tol) >= 0) &&

		   (compareDouble(pt.position.y, m_MaxPt.position.y, tol) <= 0) &&
		   (compareDouble(pt.position.y, m_MinPt.position.y, tol) >= 0) &&

		   (compareDouble(pt.position.z, m_MaxPt.position.z, tol) <= 0) &&
		   (compareDouble(pt.position.z, m_MinPt.position.z, tol) >= 0);
}

bool AABB::isIntersect(const AABB & othAABB) {
	return 
}