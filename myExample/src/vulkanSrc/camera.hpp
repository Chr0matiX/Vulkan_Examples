/*
 * Basic camera class providing a look-at and first-person camera
 *
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
		static inline const double movementSpeed = 100.0;
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

			glm::dvec3 shift = camRight * worldDX + camUp * worldDY;

			shift.z = 0.0;

			// 模拟拖动轴网，反向移动
			ptPos -= shift;
			ptTarg -= shift;
			updated = true;
		}

		void zoom(double delta) {
			if (delta > 0)
				viewSize *= (1.0 - zoomSpeed);
			else
				viewSize *= (1.0 + zoomSpeed);

			viewSize = glm::clamp(viewSize, 1.0, 100000.0);
			updated = true;
		}
};

class Camera_Old {
	private:
		float fov;
		float znear, zfar;

		void updateViewMatrix() {
			glm::mat4 currentMatrix = matrices.view;

			glm::mat4 rotM = glm::mat4(1.0f);
			glm::mat4 transM;

			rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)),
							   glm::vec3(1.0f, 0.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			glm::vec3 translation = position;
			if (flipY) {
				translation.y *= -1.0f;
			}
			transM = glm::translate(glm::mat4(1.0f), translation);

			if (type == CameraType::firstperson) {
				matrices.view = rotM * transM;
			} else {
				matrices.view = transM * rotM;
			}

			viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

			if (matrices.view != currentMatrix) {
				updated = true;
			}
		};

	public:
		enum CameraType { lookat, firstperson };
		CameraType type = CameraType::lookat;

		glm::vec3 rotation = glm::vec3();
		glm::vec3 position = glm::vec3();
		glm::vec4 viewPos = glm::vec4();

		float rotationSpeed = 2.0f;
		float movementSpeed = 40.0f;

		bool updated = true;
		bool flipY = false;

		struct {
				glm::mat4 perspective;
				glm::mat4 view;
		} matrices;

		struct {
				bool left = false;
				bool right = false;
				bool up = false;
				bool down = false;
		} keys;

		bool moving() const { return keys.left || keys.right || keys.up || keys.down; }

		float getNearClip() const { return znear; }

		float getFarClip() const { return zfar; }

		void setPerspective(float fov, float aspect, float znear, float zfar) {
			glm::mat4 currentMatrix = matrices.perspective;
			this->fov = fov;
			this->znear = znear;
			this->zfar = zfar;
			matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
			if (flipY) {
				matrices.perspective[1][1] *= -1.0f;
			}
			if (matrices.perspective != currentMatrix) {
				updated = true;
			}
		};

		void setOrthographic(float size, float aspect, float znear, float zfar) {
			glm::mat4 currentMatrix = matrices.perspective;

			float left = -size * aspect;
			float right = size * aspect;
			float bottom = size;
			float top = -size;

			matrices.perspective = glm::ortho(left, right, bottom, top, znear, zfar);

			if (flipY) {
				matrices.perspective[1][1] *= -1.0f;
			}

			if (matrices.perspective != currentMatrix) {
				updated = true;
			}
		}

		void updateAspectRatio(float aspect) {
			glm::mat4 currentMatrix = matrices.perspective;
			matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
			if (flipY) {
				matrices.perspective[1][1] *= -1.0f;
			}
			if (matrices.view != currentMatrix) {
				updated = true;
			}
		}

		void setPosition(glm::vec3 position) {
			this->position = position;
			updateViewMatrix();
		}

		void setRotation(glm::vec3 rotation) {
			this->rotation = rotation;
			updateViewMatrix();
		}

		void rotate(glm::vec3 delta) {
			this->rotation += delta;
			updateViewMatrix();
		}

		void setTranslation(glm::vec3 translation) {
			this->position = translation;
			updateViewMatrix();
		};

		void translate(glm::vec3 delta) {
			this->position += delta;
			updateViewMatrix();
		}

		void setRotationSpeed(float rotationSpeed) { this->rotationSpeed = rotationSpeed; }

		void setMovementSpeed(float movementSpeed) { this->movementSpeed = movementSpeed; }

		void update(float deltaTime) {
			updated = false;
			if (type == CameraType::firstperson) {
				if (moving()) {
					glm::vec3 camFront;
					camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
					camFront.y = sin(glm::radians(rotation.x));
					camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
					camFront = glm::normalize(camFront);

					float moveSpeed = deltaTime * movementSpeed;

					if (keys.up)
						position += camFront * moveSpeed;
					if (keys.down)
						position -= camFront * moveSpeed;
					if (keys.left)
						position -=
							glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) *
							moveSpeed;
					if (keys.right)
						position +=
							glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) *
							moveSpeed;
				}
			}
			updateViewMatrix();
		};

		// Update camera passing separate axis data (gamepad)
		// Returns true if view or position has been changed
		bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime) {
			bool retVal = false;

			if (type == CameraType::firstperson) {
				// Use the common console thumbstick layout
				// Left = view, right = move

				const float deadZone = 0.0015f;
				const float range = 1.0f - deadZone;

				glm::vec3 camFront;
				camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
				camFront.y = sin(glm::radians(rotation.x));
				camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
				camFront = glm::normalize(camFront);

				float moveSpeed = deltaTime * movementSpeed * 2.0f;
				float rotSpeed = deltaTime * rotationSpeed * 50.0f;

				// Move
				if (fabsf(axisLeft.y) > deadZone) {
					float pos = (fabsf(axisLeft.y) - deadZone) / range;
					position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
					retVal = true;
				}
				if (fabsf(axisLeft.x) > deadZone) {
					float pos = (fabsf(axisLeft.x) - deadZone) / range;
					position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) *
								pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
					retVal = true;
				}

				// Rotate
				if (fabsf(axisRight.x) > deadZone) {
					float pos = (fabsf(axisRight.x) - deadZone) / range;
					rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
					retVal = true;
				}
				if (fabsf(axisRight.y) > deadZone) {
					float pos = (fabsf(axisRight.y) - deadZone) / range;
					rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
					retVal = true;
				}
			} else {
				// todo: move code from example base class for look-at
			}

			if (retVal) {
				updateViewMatrix();
			}

			return retVal;
		}
};