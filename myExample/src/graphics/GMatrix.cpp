#include "GMatrix.h"

#include "../Utils.hpp"
#include "GPoint.h"
#include "GVector.h"

#include <algorithm>
#include <cmath>

GMatrix & GMatrix::setToIdentity() noexcept {
	m_Mat = glm::dmat4{1.0};
	return *this;
}

GMatrix & GMatrix::setToMove(const GVector & vec) noexcept {
	m_Mat[3][0] = vec.vec.x;
	m_Mat[3][1] = vec.vec.y;
	m_Mat[3][2] = vec.vec.z;
	return *this;
}

GMatrix & GMatrix::setToRotate(const GPoint & ptCenter, const GVector & vecNormal,
							   const double angle) noexcept {
	setToIdentity();

	const double c = std::cos(angle);
	const double s = std::sin(angle);
	const double t = 1.0 - c;

	m_Mat[0][0] = t * vecNormal.vec.x * vecNormal.vec.x + c;
	m_Mat[0][1] = t * vecNormal.vec.x * vecNormal.vec.y + s * vecNormal.vec.z;
	m_Mat[0][2] = t * vecNormal.vec.x * vecNormal.vec.z - s * vecNormal.vec.y;

	m_Mat[1][0] = t * vecNormal.vec.x * vecNormal.vec.y - s * vecNormal.vec.z;
	m_Mat[1][1] = t * vecNormal.vec.y * vecNormal.vec.y + c;
	m_Mat[1][2] = t * vecNormal.vec.y * vecNormal.vec.z + s * vecNormal.vec.x;

	m_Mat[2][0] = t * vecNormal.vec.x * vecNormal.vec.z + s * vecNormal.vec.y;
	m_Mat[2][1] = t * vecNormal.vec.y * vecNormal.vec.z - s * vecNormal.vec.x;
	m_Mat[2][2] = t * vecNormal.vec.z * vecNormal.vec.z + c;

	m_Mat[3][0] = ptCenter.position.x -
				  (m_Mat[0][0] * ptCenter.position.x + m_Mat[1][0] * ptCenter.position.y +
				   m_Mat[2][0] * ptCenter.position.z);
	m_Mat[3][1] = ptCenter.position.y -
				  (m_Mat[0][1] * ptCenter.position.x + m_Mat[1][1] * ptCenter.position.y +
				   m_Mat[2][1] * ptCenter.position.z);
	m_Mat[3][2] = ptCenter.position.z -
				  (m_Mat[0][2] * ptCenter.position.x + m_Mat[1][2] * ptCenter.position.y +
				   m_Mat[2][2] * ptCenter.position.z);

	return *this;
}

GMatrix GMatrix::operator*(const GMatrix & mtx) const noexcept {
	GMatrix result;
	result.m_Mat = m_Mat * mtx.m_Mat;

	return result;
}

GMatrix & GMatrix::operator*=(const GMatrix & mtx) noexcept {
	m_Mat = m_Mat * mtx.m_Mat;

	return *this;
}

GMatrix GMatrix::getInverse() const noexcept {
	GMatrix res;
	double det = glm::determinant(m_Mat);

	if (compleDouble(glm::determinant(m_Mat), 0) <= 0) {
		res.setToIdentity();
	} else {
		res.m_Mat = glm::inverse(m_Mat);
	}

	return res;
}

glm::dvec4 GMatrix::dot(const glm::dvec4 & dVec4) const noexcept {
	return m_Mat * dVec4;
}

void GMatrix::dot(glm::dvec4 & dVec4) const noexcept {
	dVec4 = m_Mat * dVec4;
}