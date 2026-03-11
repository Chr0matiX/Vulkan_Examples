#pragma once

#include <glm/glm.hpp>

class GVector;
class GMatrix;

class GPoint {
	public:
		glm::dvec3 position;

	public:
		static const GPoint origin;

		GPoint() : position{0, 0, 0} {}
		GPoint(const double x, const double y, const double z) : position{x, y, z} {}
		GPoint(const glm::dvec3 & pos) : position(pos) {}

		void set(const double x, const double y, const double z) noexcept { position = {x, y, z}; }

	public:
		GVector operator-(const GPoint & pt) const noexcept;

		GPoint operator+(const GVector & vec) const noexcept;

		GPoint & operator+=(const GVector & vec) noexcept;

		double distanceTo(const GPoint & pt) const noexcept;

		glm::vec3 to_GlmVec3() const noexcept {
			return glm::vec3{static_cast<float>(position.x), static_cast<float>(position.y),
							 static_cast<float>(position.z)};
		}

		GPoint & transformBy(const GMatrix & mtx) noexcept;
};