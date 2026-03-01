#pragma once

#include <glm/glm.hpp>

class GVector;
class GMatrix;

class GPoint {
	public:
		glm::dvec3 position;

	public:
		static const GPoint origin;

		inline GPoint(const double x, const double y, const double z) : position{x, y, z} {}

		void set(const double x, const double y, const double z) noexcept {
			position = {x, y, z};
		}

	public:
		GVector operator-(const GPoint & pt) const noexcept;

		GPoint operator+(const GVector & vec) const noexcept;
		
		GPoint & operator+=(const GVector & vec) noexcept;

		glm::dvec3 to_GlmVec3() const noexcept;

		GPoint & transformBy(const GMatrix & mtx)noexcept;
};