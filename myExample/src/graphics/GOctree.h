#pragma once

#include "GPoint.h"

class AABB {
	private:
		GPoint m_MaxPt;
		GPoint m_MinPt;

	public:
		AABB() {}
		AABB(const GPoint & maxPt, const GPoint & minPt) : m_MaxPt(maxPt), m_MinPt(minPt) {}
		~AABB() {}

		bool isPtIn(const GPoint & pt ,const double tol = 0.1);

		bool isIntersect(const AABB & othAABB);
};
