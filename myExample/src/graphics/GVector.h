#pragma once

#include <glm/glm.hpp>

class GPoint;
class GMatrix;

class GVector {
	public:
		glm::dvec3 vec;

	public:
		static const GVector xAxis;
		static const GVector yAxis;
		static const GVector zAxis;

		GVector(const double x, const double y, const double z) : vec{x, y, z} {}

		void set(const double x, const double y, const double z) noexcept { vec = {x, y, z}; }

	public:
		template <typename T> GVector operator*(const T value) {
			glm::dvec3 dvec3 = vec;
			*value;
		}

		double getLength() const noexcept;

		GVector & normalize();

		glm::dvec3 to_GlmVec3() const noexcept;

		GVector & transformBy(const GMatrix & mtx) noexcept;
};