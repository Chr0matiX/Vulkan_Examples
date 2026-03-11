#include "graphics/GGeometry.h"
#include "vulkanSrc/renderObject.h"
#include "vulkanSrc/vkContext.h"

#include <random>

int main() {
	/* std::vector<VertexInfo> vec_VertInfo{
		GCone({3000.0, 3000.0, 1000.0 / std::tan(glm::radians(45.0))}, 1000.0, 500.0, -45)
			.getVertex(),
		GCone({0.0, 0.0, 0.0}, 1000.0, 600.0, 30).getVertex(),
		GCone({3000.0, 0.0, 0.0}, 1000.0, 1000.0, 45).getVertex(),
		GCone({0.0, 3000.0, 0.0}, 1000.0, 3000.0, 30).getVertex(),
	}; */
	// vec_VertInfo.reserve(10 * 10 * 4);

	RenderObjectManager::getInstance().addRenderObject(new RenderObject(new GCone()));

	// 随机数据生成
	// 固定种子
	std::default_random_engine rSeed(12345);
	const double stepSize = 3000.0;

	double zOffset = 0;

	// 单锥
	{
		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle)));

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 圆台
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) / 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 双锥
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) * 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 全随机
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_real_distribution<double> rHeight(1000.0, 3000.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, 0 + zOffset}, rRadius(rSeed),
							  rHeight(rSeed), rHalfAngle(rSeed) * (signs ? 1 : -1))));
			}
		}
	}
	// test
	// 圆台
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) / 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 圆台
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) / 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 圆台
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) / 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}
	// 圆台
	{
		zOffset += stepSize * 2;

		std::uniform_real_distribution<double> rRadius(1000.0, 2000.0);
		std::uniform_real_distribution<double> rHalfAngle(20.0, 70.0);
		std::uniform_int_distribution<int> rSigns(0, 1);

		for (uint32_t row = 0; row < 100; ++row) {
			for (uint32_t col = 0; col < 100; ++col) {
				const auto & signs = rSigns(rSeed);
				const auto & radius = rRadius(rSeed);
				const auto & halfAngle = rHalfAngle(rSeed) * (signs ? 1 : -1);
				const auto & height = std::abs(radius / std::tan(glm::radians(halfAngle))) / 2;

				RenderObjectManager::getInstance().addRenderObject(new RenderObject(
					new GCone({col * stepSize, row * stepSize, (signs ? 0 : height) + zOffset},
							  radius, height, halfAngle)));
			}
		}
	}

	// VertexInfo vertexInfo = mergVertexInfo(vec_VertInfo, false);

	const auto & vertexInfoMerg = RenderObjectManager::getInstance().getVertexInfo();

	VkContext::getInstance().setVertex(vertexInfoMerg.vec_Vertex);
	VkContext::getInstance().setIndex(vertexInfoMerg.vec_Index);

	VkContext::getInstance().init();

	if (!VkContext::getInstance().valid())
		return 1;

	// vec_VertInfo.clear();

	VkContext::getInstance().startRenderLoop();

	return 0;
}
