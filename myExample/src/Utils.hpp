#pragma once

#include "glm/detail/type_vec.hpp"
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>

#include <ctime>
#include <iostream>
#include <stdexcept>
#include <vector>

#define SINGLETON_CLASS(className)                     \
private:                                               \
	className() = default;                             \
	~className() = default;                            \
	className(const className &) = delete;             \
	className(className &&) = delete;                  \
	className & operator=(const className &) = delete; \
	className & operator=(className &&) = delete;      \
                                                       \
public:

#define DEFAULT_FENCE_TIMEOUT 100000000000

// 定义数据结构一定要注意 GPU 侧对齐
struct /* alignas(16) */ Vertex {
		/* alignas(16) */ glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;
};

struct VertexInfo {
		std::vector<Vertex> vec_Vertex;
		std::vector<uint32_t> vec_Index;
};

struct ShaderData {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
};

inline uint32_t getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties & memoryProperty,
								   const uint32_t & memTypeBits,
								   const VkMemoryPropertyFlags & memPropertyFlags,
								   bool * memTypeFound = nullptr) {
	uint32_t memTypeBitsClone = memTypeBits;
	for (uint32_t i = 0; i < memoryProperty.memoryTypeCount; i++) {
		if ((memTypeBitsClone & 1) == 1) {
			if ((memoryProperty.memoryTypes[i].propertyFlags & memPropertyFlags) ==
				memPropertyFlags) {
				if (memTypeFound != nullptr) {
					*memTypeFound = true;
				}
				return i;
			}
		}
		memTypeBitsClone >>= 1;
	}

	if (memTypeFound) {
		*memTypeFound = false;
		return 0;
	} else {
		throw std::runtime_error("Could not find a matching memory type");
	}
}

inline std::string getCurrentTimeStr() {
	std::time_t t = std::time(nullptr);
	std::tm tm{};

	localtime_s(&tm, &t);

	char buf[20];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
	return buf;
}

#define CHECK_VK_RESULT(f) checkVkResult((f), #f, __FILE__, __LINE__)

inline void checkVkResult(const VkResult & vkResult, const char * funcName, const char * fileName,
						  const int line) {
	if (vkResult == VK_SUCCESS)
		return;

	if (vkResult > VK_SUCCESS) {
		std::cout << "[Warning]\t{" << getCurrentTimeStr() << "}\tvkResult: " << vkResult
				  << ", func: " << funcName << ", file: " << fileName << ", line: " << line
				  << std::endl;
		return;
	}

	std::cout << "[Error]\t{" << getCurrentTimeStr() << "}\tvkResult: " << vkResult
			  << ", func: " << funcName << ", file: " << fileName << ", line: " << line
			  << std::endl;
	fflush(stdout);
	exit(1);
}

struct DepthStencilRes {
		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_Memory;
		VkFormat m_Format;
};

// return: -1 0 1
inline int8_t compareDouble(const double v1, const double v2, const double tol = 0.0001) {
	if ((abs)(v1 - v2) < tol)
		return 0;

	return v1 < v2 ? -1 : 1;
}

inline glm::vec3 adjustColorToLuminance(glm::vec3 color, float targetLuma = 0.8f) {
	const glm::vec3 weights(0.2126f, 0.7152f, 0.0722f);

	float currentLuma = glm::dot(color, weights);
	if (currentLuma < 0.0001f) {
		return glm::vec3(targetLuma);
	}

	float scale = targetLuma / currentLuma;
	glm::vec3 adjustedColor = color * scale;

	float maxComponent = std::max(adjustedColor.r, std::max(adjustedColor.g, adjustedColor.b));

	if (maxComponent > 1.0f) {
		glm::vec3 normalizedColor = adjustedColor / maxComponent;
		float lumaAtMax = glm::dot(normalizedColor, weights);

		float t = (targetLuma - lumaAtMax) / (1.0f - lumaAtMax);
		t = glm::clamp(t, 0.0f, 1.0f);

		return glm::mix(normalizedColor, glm::vec3(1.0f), t);
	}

	return adjustedColor;
}

inline glm::vec3 adjustColorToLuminance_new(glm::vec3 color, float targetLuma = 0.6f) {
	const glm::vec3 weights(0.2126f, 0.7152f, 0.0722f);

	float currentLuma = glm::dot(color, weights);

	if (currentLuma < 0.0001f) {
		return glm::vec3(targetLuma);
	}

	float scale = targetLuma / currentLuma;
	glm::vec3 adjustedColor = color * scale;

	float maxC = std::max(adjustedColor.r, std::max(adjustedColor.g, adjustedColor.b));

	if (maxC > 1.0f) {
		glm::vec3 normalizedColor = adjustedColor / maxC;
		float lumaAtMax = glm::dot(normalizedColor, weights);
		float t = (targetLuma - lumaAtMax) / (1.0f - lumaAtMax);
		t = glm::clamp(t, 0.0f, 1.0f);

		return glm::mix(normalizedColor, glm::vec3(1.0f), t);
	}

	return adjustedColor;
}

inline glm::vec3 getDebugColor(const glm::dvec3 & position) {
	double gridX = std::floor(position.x);
	double gridY = std::floor(position.y);
	double gridZ = std::floor(position.z);

	auto hash = [](double x, double y, double z) {
		double dot_product = x * 12.9898 + y * 78.233 + z * 43.543;
		return glm::fract(std::sin(dot_product) * 43758.5453);
	};

	glm::vec3 rgb(static_cast<float>(hash(gridX, gridY, gridZ)),
				  static_cast<float>(hash(gridX + 1.1, gridY + 2.2, gridZ + 3.3)),
				  static_cast<float>(hash(gridX + 4.4, gridY + 5.5, gridZ + 6.6)));

	return adjustColorToLuminance(rgb);
	//return adjustColorToLuminance_new(rgb);
}

// keep 为 false，则 vec_VertexInfo 将不可用
inline VertexInfo mergVertexInfo(std::vector<VertexInfo> & vec_VertexInfo, const bool keep = true) {
	VertexInfo vertInfoRtn;
	size_t vertexCount{0}, indexCount{0};

	for (const auto & vertInfo : vec_VertexInfo) {
		vertexCount += vertInfo.vec_Vertex.size();
		indexCount += vertInfo.vec_Index.size();
	}

	vertInfoRtn.vec_Vertex.reserve(vertexCount);
	vertInfoRtn.vec_Index.reserve(indexCount);

	for (auto & vertInfo : vec_VertexInfo) {
		const size_t vertexOffset = vertInfoRtn.vec_Vertex.size();
		const size_t indexStartIdx = vertInfoRtn.vec_Index.size();

		if (keep) {
			vertInfoRtn.vec_Vertex.insert(vertInfoRtn.vec_Vertex.end(), vertInfo.vec_Vertex.begin(),
										  vertInfo.vec_Vertex.end());
			vertInfoRtn.vec_Index.insert(vertInfoRtn.vec_Index.end(), vertInfo.vec_Index.begin(),
										 vertInfo.vec_Index.end());
		} else {
			vertInfoRtn.vec_Vertex.insert(vertInfoRtn.vec_Vertex.end(),
										  std::make_move_iterator(vertInfo.vec_Vertex.begin()),
										  std::make_move_iterator(vertInfo.vec_Vertex.end()));
			vertInfoRtn.vec_Index.insert(vertInfoRtn.vec_Index.end(),
										 std::make_move_iterator(vertInfo.vec_Index.begin()),
										 std::make_move_iterator(vertInfo.vec_Index.end()));

			vertInfo.vec_Vertex.clear();
			vertInfo.vec_Index.clear();
		}

		// 间接绘制这里不需要处理便宜
		//for (size_t i = indexStartIdx; i < vertInfoRtn.vec_Index.size(); ++i)
		//	vertInfoRtn.vec_Index[i] += vertexOffset;
	}
	return vertInfoRtn;
}