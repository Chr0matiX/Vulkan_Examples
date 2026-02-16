#pragma once

#include "Utils.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <vector>

class RenderVulkan {
		SINGLETON_CLASS(RenderVulkan);
		friend class VkContext;

	private:
		/**********************************************************
		外部依赖
		**********************************************************/
		/// 效率敏感资源
		std::vector<Vertex> vec_Vertex;

		std::vector<uint32_t> vec_Index;

		///
		VkRenderPass m_renderPass{VK_NULL_HANDLE};

		struct {
				uint32_t m_Graphics{0};
				uint32_t m_Present{0};
				uint32_t m_Transfer{0};
				uint32_t m_Compute{0};
		} m_QueueIndex;

		VkDevice m_LogicalDevice{VK_NULL_HANDLE};

		uint32_t m_MaxConcurrentFrames{2};

		VkPhysicalDeviceMemoryProperties m_MemoryProperty;

		std::map<uint32_t, VkQueue> map_QIndex2Queue;

		/**********************************************************
		资源
		**********************************************************/
		VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};

		VkPipeline m_Pipeline{VK_NULL_HANDLE};

		/* struct VkCommandRes {
				VkCommandPool m_CmdPool;
				std::vector<VkCommandBuffer> vec_CmdBuffer;
		} m_CmdRes; */

		std::map<uint32_t, VkCommandPool> map_QIndex2CmdPool;

		std::vector<VkCommandBuffer> vec_FrameCmdBuffer;

		/* std::map<uint32_t, VkCommandRes> map_Index2CmdRes; */

		struct BufferRes {
				VkDeviceMemory m_Memory{VK_NULL_HANDLE};
				VkBuffer m_Buffer{VK_NULL_HANDLE};
				// uint32_t m_Count{0};
		} m_VertexBufferRes, m_IndexBufferRes;

		struct UniformRes {
				BufferRes m_BufferRes;
				VkDescriptorSet m_DescriptorSet{VK_NULL_HANDLE};
				uint8_t * m_PMapped{nullptr};
		};

		std::vector<UniformRes> vec_UniformRes;

		VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};

		VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};

	private:
		bool init();

		bool valid();

		void destroy();

		bool prepare();

		bool createVertexBufferRes();

		bool createUniformBufferRes();

		bool createDescriptorSetLayout();

		bool createDescriptorPool();

		bool createDescriptorSets();

		VkCommandPool getCmdPool(const uint32_t qIndex);

		VkCommandBuffer
		getCmdBuffer(const uint32_t qIndex,
					 const VkCommandBufferLevel & cmdBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		std::vector<VkCommandBuffer> getCmdBuffers(
			const uint32_t qIndex, const uint32_t count,
			const VkCommandBufferLevel & cmdBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		bool freeCommandBuffer(const uint32_t qIndex, const VkCommandBuffer & cmdBuffer);

		bool freeCommandBuffer(const uint32_t qIndex,
							   const std::vector<VkCommandBuffer> & vec_CmdBuffer);

		VkCommandBuffer getFrameCmdBuffer(
			const uint32_t currFrameIndex,
			const VkCommandBufferLevel & cmdBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		template <typename T>
		bool getBuffer(const std::vector<T> & vec_Value,
					   const VkBufferUsageFlags & bufferUsageFlags, VkBuffer & buffer,
					   VkDeviceMemory & bufferMemory);

		bool allocateBuffer(const uint32_t bufferSize, const VkBufferUsageFlags & bufferUsageFlags,
							const VkMemoryPropertyFlags & memPropertyFlags, VkBuffer & buffer,
							VkDeviceMemory & bufferMemory);
};

/*
静态资源初始化
同步资源获取
commandPool
顶点buffer
uniformBuffer
修饰符布局 DescriptorSetLayout
修饰符池
修饰符set
pipeline
*/