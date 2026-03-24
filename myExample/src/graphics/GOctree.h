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

		bool isPtIn(const GPoint & pt, const double tol = 0.1) const noexcept;

		bool isIntersect(const AABB & othAABB, const double tol = 0.1) const noexcept;

		void set(const GPoint & maxPt, const GPoint & minPt) noexcept;

		void add(const AABB & othAABB) noexcept;
		void add(const GPoint & pt) noexcept;

		GPoint getCenter() const noexcept;

		AABB getChildAABB(const uint8_t index) const noexcept;
};

class OctreeNode {

};

class Octree {

};