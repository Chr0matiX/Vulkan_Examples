#include "renderVulkan.h"

bool RenderVulkan::init() {
	bool rtn = false;

	do {

		rtn = true;
	} while (0);

	return rtn;
}

bool RenderVulkan::valid() {
	return false;
}

void RenderVulkan::destroy() {
	return;
}

bool RenderVulkan::prepare() {
	// 同步对象

	// CommandPool

	// 顶点数据，搬运到 GPU 中

	// UniformBuffer

	// Descriptor 资源
	// Layout
	// Pool
	// Set

	// Pipeline

	return true;
}

VkCommandBuffer RenderVulkan::getCmdBuffer(
	const uint32_t queueIndex, const size_t currFrameIndex,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {
	const auto & it_Index2CmdRes = map_Index2CmdRes.find(queueIndex);
	if (it_Index2CmdRes != map_Index2CmdRes.end())
		return it_Index2CmdRes->second.vec_CmdBuffer[currFrameIndex];

	VkCommandPoolCreateInfo cmdPoolCI{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
									  .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
									  .queueFamilyIndex = queueIndex};

	auto & cmdRes = map_Index2CmdRes.emplace(queueIndex, VkCommandRes()).first->second;

	if (vkCreateCommandPool(m_LogicalDevice, &cmdPoolCI, nullptr, &cmdRes.m_CmdPool) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	cmdRes.vec_CmdBuffer.resize(m_MaxConcurrentFrames);
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = cmdRes.m_CmdPool,
		.level = cmdBufferLevel,
		.commandBufferCount = m_MaxConcurrentFrames};
	if (vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufferAllocateInfo,
								 cmdRes.vec_CmdBuffer.data()) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return cmdRes.vec_CmdBuffer[currFrameIndex];
}

bool RenderVulkan::createVertexBuffer() {
	m_IndexRes.m_Count = static_cast<uint32_t>(vec_Index.size());

	uint32_t vertexBufferSize = static_cast<uint32_t>(vec_Vertex.size()) * sizeof(Vertex);
	uint32_t indexBufferSize = m_IndexRes.m_Count * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	VkMemoryRequirements memReqs;
	VertexRes stagingVertexRes, stagingIndexRes;
	void * data;

	// stagingVertexRes
	{
		VkBufferCreateInfo vertexBufferInfoCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
											  .size = vertexBufferSize,
											  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

		if (vkCreateBuffer(m_LogicalDevice, &vertexBufferInfoCI, nullptr,
						   &stagingVertexRes.m_Buffer) != VK_SUCCESS)
			return false;

		vkGetBufferMemoryRequirements(m_LogicalDevice, stagingVertexRes.m_Buffer, &memReqs);

		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
														  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &stagingVertexRes.m_Memory) !=
			VK_SUCCESS)
			return false;

		if (vkMapMemory(m_LogicalDevice, stagingVertexRes.m_Memory, 0, memAlloc.allocationSize, 0,
						&data) != VK_SUCCESS)
			return false;

		memcpy(data, vec_Vertex.data(), vertexBufferSize);

		vkUnmapMemory(m_LogicalDevice, stagingVertexRes.m_Memory);

		if (vkBindBufferMemory(m_LogicalDevice, stagingVertexRes.m_Buffer,
							   stagingVertexRes.m_Memory, 0) != VK_SUCCESS)
			return false;

		///
		vertexBufferInfoCI.usage =
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (vkCreateBuffer(m_LogicalDevice, &vertexBufferInfoCI, nullptr, &m_VertexRes.m_Buffer) !=
			VK_SUCCESS)
			return false;

		vkGetBufferMemoryRequirements(m_LogicalDevice, m_VertexRes.m_Buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &m_VertexRes.m_Memory) !=
			VK_SUCCESS)
			return false;
		if (vkBindBufferMemory(m_LogicalDevice, m_VertexRes.m_Buffer, m_VertexRes.m_Memory, 0) !=
			VK_SUCCESS)
			return false;
	}

	// stagingIndexRes
	{
		VkBufferCreateInfo indexbufferCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
										 .size = indexBufferSize,
										 .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
		if (vkCreateBuffer(m_LogicalDevice, &indexbufferCI, nullptr, &stagingIndexRes.m_Buffer) !=
			VK_SUCCESS)
			return false;

		vkGetBufferMemoryRequirements(m_LogicalDevice, stagingIndexRes.m_Buffer, &memReqs);

		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
														  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &stagingIndexRes.m_Memory) !=
			VK_SUCCESS)
			return false;

		if (vkMapMemory(m_LogicalDevice, stagingIndexRes.m_Memory, 0, indexBufferSize, 0, &data) !=
			VK_SUCCESS)
			return false;

		memcpy(data, vec_Index.data(), indexBufferSize);

		vkUnmapMemory(m_LogicalDevice, stagingIndexRes.m_Memory);

		if (vkBindBufferMemory(m_LogicalDevice, stagingIndexRes.m_Buffer, stagingIndexRes.m_Memory,
							   0) != VK_SUCCESS)
			return false;

		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (vkCreateBuffer(m_LogicalDevice, &indexbufferCI, nullptr, &m_IndexRes.m_Buffer) !=
			VK_SUCCESS)
			return false;

		vkGetBufferMemoryRequirements(m_LogicalDevice, m_IndexRes.m_Buffer, &memReqs);

		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &m_IndexRes.m_Memory) !=
			VK_SUCCESS)
			return false;
		if (vkBindBufferMemory(m_LogicalDevice, m_IndexRes.m_Buffer, m_IndexRes.m_Memory, 0) !=
			VK_SUCCESS)
			return false;
	}

	return true;
}