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

		GVector() : vec{0, 0, 0} {}
		GVector(const double x, const double y, const double z) : vec{x, y, z} {}
		GVector(const glm::dvec3 & dVec3) : vec{dVec3} {}

		void set(const double x, const double y, const double z) noexcept { vec = {x, y, z}; }

	public:
		template <typename T> GVector operator*(const T value) const {
			return GVector(vec * static_cast<double>(value));
		}

		template <typename T> GVector operator*=(const T value) {
			return vec = vec * static_cast<double>(value);
		}

		GVector operator+(const GVector & other) const noexcept { return GVector(vec + other.vec); }
		GVector operator+=(const GVector & other) noexcept { return vec = vec + other.vec; }

		GVector operator-(const GVector & other) const noexcept { return GVector(vec - other.vec); }
		GVector operator-=(const GVector & other) noexcept { return vec = vec - other.vec; }

		double dot(const GVector & other) const noexcept { return glm::dot(vec, other.vec); }

		GVector cross(const GVector & other) const noexcept {
			return GVector(glm::cross(vec, other.vec));
		}

		double getLength() const noexcept;

		GVector & normalize() noexcept;

		GVector getNormalize() const noexcept;

		double angleTo(const GVector & other, const GVector vecNormal) const noexcept;
		double angleTo(const GVector & other) const noexcept {
			return angleTo(other, cross(other));
		}

		glm::vec3 to_GlmVec3() const noexcept {
			return glm::vec3{static_cast<float>(vec.x), static_cast<float>(vec.y),
							 static_cast<float>(vec.z)};
		}

		GVector & transformBy(const GMatrix & mtx) noexcept;
};