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
	// 同步对象（后面再实现）

	// CommandPool（懒加载）

	// 顶点数据，搬运到 GPU 中
	if (!createVertexBufferRes())
		return false;

	// UniformBuffer

	// Descriptor 资源
	// Layout
	// Pool
	// Set

	// Pipeline

	return true;
}

VkCommandPool RenderVulkan::getCmdPool(const uint32_t qIndex) {
	const auto & it_QIndex2CmdPool = map_QIndex2CmdPool.find(qIndex);
	if (it_QIndex2CmdPool != map_QIndex2CmdPool.end())
		return it_QIndex2CmdPool->second;

	auto & cmdPool = map_QIndex2CmdPool.emplace(qIndex, VkCommandPool()).first->second;

	VkCommandPoolCreateInfo cmdPoolCI{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = qIndex,
	};

	CHECK_VK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolCI, nullptr, &cmdPool));

	return cmdPool;
}

VkCommandBuffer RenderVulkan::getCmdBuffer(
	const uint32_t qIndex,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	VkCommandBuffer rtnCmd;

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = getCmdPool(qIndex),
		.level = cmdBufferLevel,
		.commandBufferCount = 1,
	};

	CHECK_VK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufferAllocateInfo, &rtnCmd));

	return rtnCmd;
}

std::vector<VkCommandBuffer> RenderVulkan::getCmdBuffers(
	const uint32_t qIndex, const uint32_t count,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	std::vector<VkCommandBuffer> vec_RtnCmd;
	vec_RtnCmd.resize(count);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = getCmdPool(qIndex),
		.level = cmdBufferLevel,
		.commandBufferCount = count,
	};

	CHECK_VK_RESULT(
		vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufferAllocateInfo, vec_RtnCmd.data()));

	return vec_RtnCmd;
}

bool RenderVulkan::freeCommandBuffer(const uint32_t qIndex, const VkCommandBuffer & cmdBuffer) {

	if (!map_QIndex2CmdPool.contains(qIndex))
		return false;

	vkFreeCommandBuffers(m_LogicalDevice, getCmdPool(qIndex), 1, &cmdBuffer);

	return true;
}

bool RenderVulkan::freeCommandBuffer(const uint32_t qIndex,
									 const std::vector<VkCommandBuffer> & vec_CmdBuffer) {

	if (!map_QIndex2CmdPool.contains(qIndex))
		return false;

	vkFreeCommandBuffers(m_LogicalDevice, getCmdPool(qIndex), vec_CmdBuffer.size(),
						 vec_CmdBuffer.data());

	return true;
}

VkCommandBuffer RenderVulkan::getFrameCmdBuffer(
	const uint32_t currFrameIndex,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	if (currFrameIndex >= m_MaxConcurrentFrames)
		return {};

	if (vec_FrameCmdBuffer.empty())
		vec_FrameCmdBuffer =
			std::move(getCmdBuffers(m_QueueIndex.m_Graphics, m_MaxConcurrentFrames));

	return vec_FrameCmdBuffer[currFrameIndex];
}

bool RenderVulkan::createVertexBufferRes() {
	if (!getBuffer(vec_Vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_VertexBufferRes.m_Buffer,
				   m_VertexBufferRes.m_Memory))
		return false;

	if (!getBuffer(vec_Vertex, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_IndexBufferRes.m_Buffer,
				   m_IndexBufferRes.m_Memory))
		return false;

	// m_IndexBufferRes.m_Count = static_cast<uint32_t>(vec_Vertex.size());

	return true;
}

template <typename T>
bool RenderVulkan::getBuffer(const std::vector<T> & vec_Value,
							 const VkBufferUsageFlags & bufferUsageFlags, VkBuffer & buffer,
							 VkDeviceMemory & bufferMemory) {
	/// Host-Visible
	if (vec_Value.empty())
		return false;

	uint32_t bufferSize = static_cast<uint32_t>(vec_Value.size()) * sizeof(T);

	BufferRes stagingBufferRes;
	if (!allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						stagingBufferRes.m_Buffer, stagingBufferRes.m_Memory))
		return false;

	void * data;
	CHECK_VK_RESULT(
		vkMapMemory(m_LogicalDevice, stagingBufferRes.m_Memory, 0, bufferSize, 0, &data));

	memcpy(data, vec_Value.data(), bufferSize);

	vkUnmapMemory(m_LogicalDevice, stagingBufferRes.m_Memory);

	/// Device-Local
	if (!allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlags,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory))
		return false;

	/// Copy command
	VkCommandBuffer copyCmd = getCmdBuffer(m_QueueIndex.m_Transfer);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	if (vkBeginCommandBuffer(copyCmd, &cmdBufferBeginInfo))
		return false;

	VkBufferCopy copyRegion{.size = bufferSize};
	vkCmdCopyBuffer(copyCmd, stagingBufferRes.m_Buffer, buffer, 1, &copyRegion);

	CHECK_VK_RESULT(vkEndCommandBuffer(copyCmd));

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &copyCmd,
	};

	VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = 0};
	VkFence fence;
	CHECK_VK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCI, nullptr, &fence));

	CHECK_VK_RESULT(
		vkQueueSubmit(map_QIndex2Queue[m_QueueIndex.m_Transfer], 1, &submitInfo, fence));

	CHECK_VK_RESULT(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	// Destroy
	vkDestroyFence(m_LogicalDevice, fence, nullptr);
	freeCommandBuffer(m_QueueIndex.m_Transfer, copyCmd);

	vkDestroyBuffer(m_LogicalDevice, stagingBufferRes.m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, stagingBufferRes.m_Memory, nullptr);

	return true;
}

bool RenderVulkan::allocateBuffer(const uint32_t bufferSize,
								  const VkBufferUsageFlags & bufferUsageFlags,
								  const VkMemoryPropertyFlags & memPropertyFlags, VkBuffer & buffer,
								  VkDeviceMemory & bufferMemory) {

	VkBufferCreateInfo bufferInfoCI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,
		.usage = bufferUsageFlags,
	};

	CHECK_VK_RESULT(vkCreateBuffer(m_LogicalDevice, &bufferInfoCI, nullptr, &buffer));

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memReqs.size,
		.memoryTypeIndex =
			getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits, memPropertyFlags),
	};

	CHECK_VK_RESULT(vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &bufferMemory));

	CHECK_VK_RESULT(vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0));

	return true;
}

bool RenderVulkan::createUniformBufferRes() {
	VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = 0,
		.memoryTypeIndex = 0,
	};

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = sizeof(ShaderData),
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	};

	vec_UniformRes.resize(m_MaxConcurrentFrames);

	VkMemoryRequirements memReqs;

	for (uint32_t i = 0; i < m_MaxConcurrentFrames; i++) {
		auto & uniformRes = vec_UniformRes[i];

		CHECK_VK_RESULT(vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr,
									   &uniformRes.m_BufferRes.m_Buffer));

		vkGetBufferMemoryRequirements(m_LogicalDevice, uniformRes.m_BufferRes.m_Buffer, &memReqs);

		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
														   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		CHECK_VK_RESULT(vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr,
										 &uniformRes.m_BufferRes.m_Memory));

		CHECK_VK_RESULT(vkBindBufferMemory(m_LogicalDevice, uniformRes.m_BufferRes.m_Buffer,
										   uniformRes.m_BufferRes.m_Memory, 0));

		CHECK_VK_RESULT(vkMapMemory(m_LogicalDevice, uniformRes.m_BufferRes.m_Memory, 0,
									sizeof(ShaderData), 0, (void **)&uniformRes.m_PMapped));
	}

	return !vec_UniformRes.empty();
}

bool RenderVulkan::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding layoutBinding{
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = 1,
		.pBindings = &layoutBinding,
	};

	CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_LogicalDevice, &descriptorLayoutCI, nullptr,
												&m_DescriptorSetLayout));

	return true;
}

bool RenderVulkan::createDescriptorPool() {
	VkDescriptorPoolSize descriptorTypeCounts[1]{};
	descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorTypeCounts[0].descriptorCount = m_MaxConcurrentFrames;

	VkDescriptorPoolCreateInfo descriptorPoolCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.maxSets = m_MaxConcurrentFrames,
		.poolSizeCount = 1,
		.pPoolSizes = descriptorTypeCounts,
	};

	CHECK_VK_RESULT(
		vkCreateDescriptorPool(m_LogicalDevice, &descriptorPoolCI, nullptr, &m_DescriptorPool));

	return true;
}

bool RenderVulkan::createDescriptorSets() {

	for (uint32_t i = 0; i < m_MaxConcurrentFrames; i++) {
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &m_DescriptorSetLayout,
		};

		CHECK_VK_RESULT(vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo,
												 &vec_UniformRes[i].m_DescriptorSet));

		VkDescriptorBufferInfo bufferInfo{
			.buffer = vec_UniformRes[i].m_BufferRes.m_Buffer,
			.range = sizeof(ShaderData),
		};

		VkWriteDescriptorSet writeDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vec_UniformRes[i].m_DescriptorSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo,
		};

		vkUpdateDescriptorSets(m_LogicalDevice, 1, &writeDescriptorSet, 0, nullptr);
	}

	return true;
}