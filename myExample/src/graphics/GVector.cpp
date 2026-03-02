#include "GVector.h"

#include "../Utils.hpp"
#include "GMatrix.h"
#include "GPoint.h"

#include <cmath>

const GVector GVector::xAxis{1, 0, 0};
const GVector GVector::yAxis{0, 1, 0};
const GVector GVector::zAxis{0, 0, 1};

double GVector::getLength() const noexcept {
	return glm::length(vec);
}

GVector & GVector::normalize() noexcept {
	const double & length = getLength();

	if (compleDouble(length, 0) != 0)
		vec = glm::normalize(vec);

	return *this;
}

GVector GVector::getNormalize() const noexcept {
	GVector vecTmp{*this};
	vecTmp.normalize();
	return vecTmp;
}

double GVector::angleTo(const GVector & other, const GVector vecNormal) const noexcept {
	glm::dvec3 normal = glm::normalize(vecNormal.vec);

	glm::dvec3 projThis = vec - glm::dot(vec, normal) * normal;
	glm::dvec3 projOther = other.vec - glm::dot(other.vec, normal) * normal;

	double lenThis = glm::length(projThis);
	double lenOther = glm::length(projOther);
	if ((compleDouble(lenThis, 0) <= 0) || (compleDouble(lenOther, 0) <= 0))
		return 0.0;

	projThis /= lenThis;
	projOther /= lenOther;

	// cos
	double dotVal = glm::dot(projThis, projOther);
	dotVal = std::clamp(dotVal, -1.0, 1.0);
	double angle = std::acos(dotVal);

	glm::dvec3 crossProd = glm::cross(projThis, projOther);
	if (glm::dot(normal, crossProd) < 0.0)
		angle = -angle;

	return angle;
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