#pragma once

#include <glm/glm.hpp>

#include <algorithm>

class GVector;
class GPoint;

class GMatrix {
	private:
		glm::dmat4 m_Mat{1.0};

	public:
		GMatrix() {};

		GMatrix(const GMatrix & mtx) {
			std::copy(&mtx.m_Mat[0][0], &mtx.m_Mat[0][0] + 16, &m_Mat[0][0]);
		}

		GMatrix & setToIdentity() noexcept;

		GMatrix & setToMove(const GVector & vec) noexcept;

		/*
		vecNormal 必须为单位向量
		旋转方向固定为逆时针
		*/
		GMatrix & setToRotate(const GPoint & ptCenter, const GVector & vecNormal,
							  const double angle) noexcept;

		GMatrix operator*(const GMatrix & mtx) const noexcept;

		GMatrix & operator*=(const GMatrix & mtx) noexcept;

		GMatrix getInverse() const noexcept;

		glm::dvec4 dot(const glm::dvec4 & dVec4) const noexcept;
		void dot(glm::dvec4 & dVec4) const noexcept;
};