#include "GVector.h"

#include "../Utils.hpp"
#include "GMatrix.h"
#include "GPoint.h"

#include <cmath>

const GVector GVector::xAxis{1, 0, 0};
const GVector GVector::yAxis{0, 1, 0};
const GVector GVector::zAxis{0, 0, 1};

double GVector::getLength() const noexcept {
	return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

GVector & GVector::normalize() {
	const double & length = getLength();

	if (compleDouble(length, 0) == 0)
		return *this;

	vec.x /= length;
	vec.y /= length;
	vec.z /= length;

	return *this;
}

glm::dvec3 GVector::to_GlmVec3() const noexcept {
	return vec;
}

GVector & GVector::transformBy(const GMatrix & mtx) noexcept {
	glm::dvec4 dVec4{vec.x, vec.y, vec.z, 0.0f};
	mtx.dot(dVec4);

	vec = {dVec4.x, dVec4.y, dVec4.z};

	return *this;
}