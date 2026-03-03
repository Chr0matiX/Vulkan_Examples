#include "GPoint.h"

#include "GMatrix.h"
#include "GVector.h"

const GPoint GPoint::origin{0, 0, 0};

GVector GPoint::operator-(const GPoint & pt) const noexcept {
	return GVector{position.x - pt.position.x, position.y - pt.position.y,
				   position.z - pt.position.z};
}

GPoint GPoint::operator+(const GVector & vec) const noexcept {
	return GPoint{position.x + vec.vec.x, position.y + vec.vec.y, position.z + vec.vec.z};
}

GPoint & GPoint::operator+=(const GVector & vec) noexcept {
	position.x += vec.vec.x;
	position.y += vec.vec.y;
	position.z += vec.vec.z;

	return *this;
}

double GPoint::distanceTo(const GPoint & pt) const noexcept {
	return (*this - pt).getLength();
}

GPoint & GPoint::transformBy(const GMatrix & mtx) noexcept {
	glm::dvec4 dVec4{position.x, position.y, position.z, 1.0f};
	mtx.dot(dVec4);

	position = {dVec4.x, dVec4.y, dVec4.z};

	return *this;
}