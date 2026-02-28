#pragma once

#include "glm/detail/type_vec.hpp"
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>

#include <ctime>
#include <iostream>
#include <stdexcept>

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

struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;
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