#pragma once

#include "vulkan/vulkan.h"

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

struct Vertex {
		float position[3];
		float color[3];
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