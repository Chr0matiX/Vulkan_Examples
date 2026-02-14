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

		/**********************************************************
		资源
		**********************************************************/
		VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};

		VkPipeline m_Pipeline{VK_NULL_HANDLE};

		struct VkCommandRes {
				VkCommandPool m_CmdPool;
				std::vector<VkCommandBuffer> vec_CmdBuffer;
		} m_CmdRes;

		std::map<uint32_t, VkCommandRes> map_Index2CmdRes;

		struct VertexRes {
				VkDeviceMemory m_Memory{VK_NULL_HANDLE};
				VkBuffer m_Buffer{VK_NULL_HANDLE};
				uint32_t m_Count{0};
		} m_VertexRes, m_IndexRes;

	private:
		bool init();

		bool valid();

		void destroy();

		bool prepare();

		VkCommandBuffer
		getCmdBuffer(const uint32_t queueIndex, const size_t currFrameIndex,
					 const VkCommandBufferLevel & cmdBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		bool createVertexBuffer();
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