#pragma once

#include "GMatrix.h"
#include "GPoint.h"
#include "GVector.h"

#include <glm/gtc/constants.hpp>

#include <vector>

class GCurve {
	private:
	protected:
		GCurve() {}

	public:
		virtual ~GCurve() {}

		virtual GPoint getPtBegin() const noexcept = 0;

		virtual GPoint getPtEnd() const noexcept = 0;

		virtual GPoint getPtAt(const double ratio) const noexcept = 0;

		virtual double getLength() const noexcept = 0;

		virtual GPoint getClosestPt(const GPoint & pt,
									const bool extend = false) const noexcept = 0;

		virtual GVector getTangentAt(const double ratio) const noexcept = 0;
		virtual GVector getTangentAt(const GPoint & pt) const noexcept = 0;

		virtual std::vector<GPoint> getVertex() const noexcept = 0;
};

class GLineSeg final : GCurve {
	private:
		GPoint m_PtBegin;
		GPoint m_PtEnd;

	public:
		GLineSeg() {}

		GLineSeg(const GPoint & ptBegin, const GPoint & ptEnd)
			: m_PtBegin(ptBegin), m_PtEnd(ptEnd) {}

		~GLineSeg() override {}

		GPoint getPtBegin() const noexcept override { return m_PtBegin; }

		GPoint getPtEnd() const noexcept override { return m_PtEnd; }

		GPoint getPtAt(const double ratio) const noexcept override {
			GPoint ptTmp{m_PtBegin};
			ptTmp += ((m_PtEnd - m_PtBegin) * ratio);
			return ptTmp;
		}

		double getLength() const noexcept override { return m_PtBegin.distanceTo(m_PtEnd); }

		GPoint getClosestPt(const GPoint & pt, const bool extend = false) const noexcept override;

		GVector getTangentAt(const double ratio) const noexcept override {
			return (m_PtEnd - m_PtBegin).normalize();
		}
		GVector getTangentAt(const GPoint & pt) const noexcept override {
			return (m_PtEnd - m_PtBegin).normalize();
		}

		std::vector<GPoint> getVertex() const noexcept override {
			return std::vector<GPoint>{m_PtBegin, m_PtEnd};
		}
};

class GArc final : GCurve {
	private:
		GPoint m_PtCenter;
		double m_Radius{0.0f};
		// 默认参照 m_VecNormal 代表的平面，逆时针绘制
		GVector m_VecNormal;
		GVector m_VecBegin;
		GVector m_VecEnd;

	public:
		GArc() {};

		GArc(const GPoint & ptCenter, const double raduis, const GVector & vecNormal,
			 const GVector & vecBegin, const GVector & vecEnd)
			: m_PtCenter(ptCenter), m_Radius(raduis), m_VecNormal(vecNormal.getNormalize()),
			  m_VecBegin(vecBegin.getNormalize()), m_VecEnd(vecEnd.getNormalize()) {}

		GPoint getPtBegin() const noexcept override {
			GPoint ptTmp{m_PtCenter};
			ptTmp += m_VecBegin * m_Radius;
			return ptTmp;
		}

		GPoint getPtEnd() const noexcept override {
			GPoint ptTmp{m_PtCenter};
			ptTmp += m_VecEnd * m_Radius;
			return ptTmp;
		}

		GPoint getPtAt(const double ratio) const noexcept override {
			GPoint ptTmp{m_PtCenter};
			GVector vecTmp{m_VecBegin};
			vecTmp.transformBy(GMatrix().setToRotate(
				m_PtCenter, m_VecNormal, m_VecBegin.angleTo(m_VecEnd, m_VecNormal) * ratio));

			ptTmp += vecTmp;
			return ptTmp;
		}

		virtual double getLength() const noexcept override {
			return glm::pi<double>() * 2 * m_Radius * m_VecBegin.angleTo(m_VecEnd) / 360;
		}

		GPoint getClosestPt(const GPoint & pt, const bool extend = false) const noexcept override;

		GVector getTangentAt(const double ratio) const noexcept override {
			return (getPtAt(ratio) - m_PtCenter).cross(m_VecNormal).normalize();
		}
		GVector getTangentAt(const GPoint & pt) const noexcept override {
			return (pt - m_PtCenter).cross(m_VecNormal).normalize();
		}

		std::vector<GPoint> getVertex() const noexcept override;
};