/*
 * Basic camera class providing a look-at and first-person camera
 *
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "../Utils.hpp"
#include "../graphics/GCurve.h"
#include "../graphics/GPoint.h"
#include "../graphics/GVector.h"

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
	public:
		struct {
				bool left = false;
				bool right = false;
				bool up = false;
				bool down = false;
		} bKeys;

		struct {
				bool left = false;
				bool right = false;
				bool mid = false;
		} mKeys;

	private:
		// const
		static inline const double rotationSpeed = 0.2;
		static inline const double movementSpeed = 1000.0;
		static inline const double zoomSpeed = 0.2;

		static inline const glm::dvec3 worldUp{0.0, 0.0, 1.0};

		// 外部传入
		double zNear{0.0};
		double zFar{0.0};

		double viewSize{0.0};
		double windowWidth{0.0};
		double windowHeight{0.0};

		glm::dvec3 ptPos;
		glm::dvec3 ptTarg;

		// 内部数据
		bool updated{false};

		glm::dvec3 camRight;
		glm::dvec3 camUp;
		glm::dvec3 camForward;

		double yaw = 0;	  // 水平
		double pitch = 0; // 俯仰

	private:
		bool keyMoving() const { return bKeys.left || bKeys.right || bKeys.up || bKeys.down; }

	public:
		Camera() {}
		Camera(const glm::dvec3 & posPt, const glm::dvec3 & targPt, const double zN,
			   const double zF, const double size, const double windowW, const double windowH)
			: ptPos(posPt), ptTarg(targPt), zNear(zN), zFar(zF), viewSize(size),
			  windowWidth(windowW), windowHeight(windowH) {

			camForward = glm::normalize(ptTarg - ptPos);
			camRight = glm::normalize(glm::cross(camForward, worldUp));
			camUp = glm::normalize(glm::cross(camRight, camForward));

			glm::dvec3 dir = ptPos - ptTarg;
			double r = glm::length(dir);
			pitch = glm::degrees(asin(dir.z / r));
			yaw = glm::degrees(atan2(dir.y, dir.x));
		}

		glm::mat4 getPerspectiveMtx() {
			double halfW = viewSize * (windowWidth / windowHeight);
			double halfH = viewSize;
			glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, zNear, zFar);
			// y轴需要翻转
			proj[1][1] *= -1.0f;

			// 宏定义污染
			// proj[3][2] = -zNear / (zFar - zNear);

			return proj;
		}

		glm::mat4 getViewMtx() { return glm::mat4(glm::lookAt(ptPos, ptTarg, worldUp)); }

		bool update(double deltaTime) {
			bool stateChanged = false;

			if (keyMoving()) {
				glm::dvec3 moveVec(0.0);

				if (bKeys.left)
					moveVec -= camRight;
				if (bKeys.right)
					moveVec += camRight;
				if (bKeys.up)
					moveVec += camUp;
				if (bKeys.down)
					moveVec -= camUp;

				moveVec.z = 0.0;

				glm::dvec3 shift = glm::normalize(moveVec) * movementSpeed * deltaTime;
				ptPos += shift;
				ptTarg += shift;
				updated = true;
			}

			stateChanged = updated;
			updated = false;
			return stateChanged;
		}

		// mouse
		void rotate(double deltaX, double deltaY) {
			yaw += deltaX * rotationSpeed;
			pitch += deltaY * rotationSpeed;

			pitch = glm::clamp(pitch, -85.0, 85.0);

			updatePositionFromAngles();

			camForward = glm::normalize(ptTarg - ptPos);
			camRight = glm::normalize(glm::cross(camForward, worldUp));
			camUp = glm::cross(camRight, camForward);

			updated = true;
		}

		void updatePositionFromAngles() {
			double r = glm::distance(ptPos, ptTarg);
			double pRad = glm::radians(pitch);
			double yRad = glm::radians(yaw);

			ptPos.x = ptTarg.x + r * cos(pRad) * cos(yRad);
			ptPos.y = ptTarg.y + r * cos(pRad) * sin(yRad);
			ptPos.z = ptTarg.z + r * sin(pRad);
		}

		void pan(double dx, double dy) {
			double worldDX = (dx / windowWidth) * viewSize * (windowWidth / windowHeight) * 2.0;
			double worldDY = (dy / windowHeight) * viewSize * 2.0;

			glm::dvec3 shift = -camRight * worldDX + camUp * worldDY;

			shift.z = 0.0;

			ptPos += shift;
			ptTarg += shift;
			updated = true;
		}

		void zoom(double delta) {
			if (delta > 0)
				viewSize *= (1.0 - zoomSpeed);
			else
				viewSize *= (1.0 + zoomSpeed);

			viewSize = glm::clamp(viewSize, 1.0, 1000000.0);
			updated = true;
		}

		bool isPtIn(const GPoint & ptSrc) {
			GVector vecForward{camForward};
			vecForward.normalize();

			GPoint ptStart{ptPos};
			GPoint ptEnd{ptPos};

			ptStart += vecForward * zNear;
			ptEnd += vecForward * zFar;

			GLineSeg lineSegViewSpace{ptStart, ptEnd};
			const auto & lineSegLength = zFar - zNear;

			const auto & ptClosest = lineSegViewSpace.getClosestPt(ptSrc);
			const auto & vecPt = ptSrc - ptClosest;

			const double debugRatio = 0.7;

			if ((compareDouble(std::abs(vecPt.dot(GVector(camRight))), windowWidth * debugRatio) >
				 0) ||
				(compareDouble(std::abs(vecPt.dot(GVector(camUp))), windowHeight * debugRatio) > 0))
				return false;

			const auto & ptDis = ptClosest.distanceTo(ptStart);

			if ((compareDouble(ptDis, lineSegLength) > 0) || compareDouble(ptDis, 0) < 0)
				return false;

			return true;
		}
};
