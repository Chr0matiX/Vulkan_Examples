#pragma once

#include "../Utils.hpp"
#include "GCurve.h"
#include "GPoint.h"
#include "GVector.h"

#include <vector>
#include <memory>

class GGeometry {
	public:
		virtual ~GGeometry() {};
		virtual VertexInfo getVertex() const noexcept = 0;
};

class GCone : public GGeometry {
	private:
		GPoint m_PtApex;
		GVector m_VecAxis{GVector::zAxis};
		double m_HalfAngle = 0.0; // halfAngle > 0 尖角朝上，反之朝下

		double m_TopH = 0.0;
		double m_BtmH = 0.0;

	public:
		GCone() {}

		GCone(const GPoint & ptApex, const GVector & vecAxis, const double halfAngle,
			  const double topH, const double btmH)
			: m_PtApex(ptApex), m_VecAxis(vecAxis), m_HalfAngle(halfAngle), m_TopH(topH),
			  m_BtmH(btmH) {}

		GCone(const GPoint & ptBtmCenter, const double radius, const double height,
			  const double halfAngle);

		std::vector<std::unique_ptr<GCurve>> getCircle(const double height) const noexcept;

		VertexInfo getVertex() const noexcept override;
};